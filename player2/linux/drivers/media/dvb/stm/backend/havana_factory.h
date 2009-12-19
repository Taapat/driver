/************************************************************************
COPYRIGHT (C) STMicroelectronics 2005

Source file name : havana_factory.h
Author :           Julian

Definition of the implementation of havana player module for havana.


Date        Modification                                    Name
----        ------------                                    --------
16-Feb-07   Created                                         Julian

************************************************************************/

#ifndef H_HAVANA_FACTORY
#define H_HAVANA_FACTORY

#include "player.h"
#include "havana_player.h"

/// Player wrapper class responsible for system instanciation.
class HavanaFactory_c
{
private:

    class HavanaFactory_c*      NextFactory;
    const char*                 Id;
    const char*                 SubId;
    PlayerStreamType_t          PlayerStreamType;
    PlayerComponent_t           PlayerComponent;
    unsigned int                FactoryVersion;
    void*                      (*Factory)                      (void);

public:

                                HavanaFactory_c                (void);
                               ~HavanaFactory_c                (void);
    HavanaStatus_t              Init                           (class HavanaFactory_c*  FactoryList,
                                                                const char*             Id,
                                                                const char*             SubId,
                                                                PlayerStreamType_t      StreamType,
                                                                PlayerComponent_t       Component,
                                                                unsigned int            Version,
                                                                void*                  (*NewFactory)   (void));
    HavanaStatus_t              ReLink                         (class HavanaFactory_c*  FactoryList);
    bool                        CanBuild                       (const char*             Id,
                                                                const char*             SubId,
                                                                PlayerStreamType_t      StreamType,
                                                                PlayerComponent_t       Component);
    HavanaStatus_t              Build                          (void**                  Class);
    class HavanaFactory_c*      Next                           (void);
    unsigned int                Version                        (void);

};
#endif

