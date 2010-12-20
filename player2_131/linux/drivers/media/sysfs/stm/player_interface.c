/************************************************************************
COPYRIGHT (C) STMicroelectronics 2003

Source file name : player_interface.c - player interface
Author :           Julian


Date        Modification                                    Name
----        ------------                                    --------
28-Apr-08   Created                                         Julian

************************************************************************/

#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/bpa2.h>
#include <linux/mutex.h>

#include "sysfs_module.h"
#include "player_sysfs.h"
#include "player_interface.h"
#include "player_interface_ops.h"

/* The player interface context provides access to the STM streaming engine (Player2) */
struct PlayerInterfaceContext_s
{
    char*                               Name;
    struct player_interface_operations* Ops;
};

/*{{{  register_player_interface*/
static struct PlayerInterfaceContext_s*         PlayerInterface;

int register_player_interface  (char*                                   Name,
                                struct player_interface_operations*     Ops)
{
    if (PlayerInterface == NULL)
    {
        INTERFACE_ERROR ("Cannot register player interface  %s - not created\n", Name);
        return -ENOMEM;
    }
    PlayerInterface->Ops        = Ops;
    PlayerInterface->Name       = kmalloc (strlen (Name) + 1,  GFP_KERNEL);
    if (PlayerInterface->Name != NULL)
        strcpy (PlayerInterface->Name, Name);

    SysfsInit   ();     /* This must be done after the player has registered itself */

    return 0;

}
EXPORT_SYMBOL(register_player_interface);
/*}}}  */

/*{{{  PlayerInterfaceInit*/
int PlayerInterfaceInit (void)
{
    PlayerInterface     = kmalloc (sizeof (struct PlayerInterfaceContext_s), GFP_KERNEL);
    if (PlayerInterface == NULL)
    {
        INTERFACE_ERROR("Unable to create player interface context - no memory\n");
        return -ENOMEM;
    }

    memset (PlayerInterface, 0, sizeof (struct PlayerInterfaceContext_s));

    PlayerInterface->Ops       = NULL;

    return 0;
}
/*}}}  */
/*{{{  PlayerInterfaceDelete*/
int PlayerInterfaceDelete ()
{
    if (PlayerInterface->Name != NULL)
        kfree (PlayerInterface->Name);
    PlayerInterface->Name       = NULL;

    kfree (PlayerInterface);
    PlayerInterface     = NULL;
    return 0;
}
/*}}}  */

/*{{{  ComponentGetAttribute*/
int ComponentGetAttribute      (player_component_handle_t       Component,
                                const char*                     Attribute,
                                union attribute_descriptor_u*   Value)
{
    int         Result  = 0;

    Result = PlayerInterface->Ops->component_get_attribute (Component, Attribute, Value);
    if (Result < 0)
        INTERFACE_ERROR("component_get_attribute failed\n");

    return Result;
}
/*}}}  */
/*{{{  ComponentSetAttribute*/
int ComponentSetAttribute      (player_component_handle_t       Component,
                                const char*                     Attribute,
                                union attribute_descriptor_u*   Value)
{
    int         Result  = 0;

    INTERFACE_DEBUG("\n");

    Result = PlayerInterface->Ops->component_set_attribute (Component, Attribute, Value);
    if (Result < 0)
        INTERFACE_ERROR("component_set_attribute failed\n");

    return Result;
}
/*}}}  */
/*{{{  PlayerRegisterEventSignalCallback*/
player_event_signal_callback PlayerRegisterEventSignalCallback (player_event_signal_callback    Callback)
{
    return PlayerInterface->Ops->player_register_event_signal_callback (Callback);
}
/*}}}  */

