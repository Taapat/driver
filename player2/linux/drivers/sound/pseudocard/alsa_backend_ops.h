/************************************************************************
COPYRIGHT (C) STMicroelectronics 2007

Source file name : alsa_backend_ops.h - player device definitions
Author :           Daniel

Date        Modification                                    Name
----        ------------                                    --------
19-Jul-07   Created                                         Daniel

************************************************************************/

#ifndef H_ALSA_BACKEND_OPS
#define H_ALSA_BACKEND_OPS

typedef void           *component_handle_t;
typedef void           (*substream_callback_t)(void *, unsigned int);

struct alsa_substream_descriptor
{
    void *hw_buffer;
    unsigned int hw_buffer_size;
    
    unsigned int channels;
    unsigned int sampling_freq;
    unsigned int bytes_per_sample;
    
    substream_callback_t callback;
    void *user_data;
};

struct alsa_backend_operations
{
    struct module                      *owner;
    
    int (*mixer_get_instance)          (int StreamId, component_handle_t* Classoid);                                 
    int (*mixer_set_module_parameters) (component_handle_t component, void *data, unsigned int size);
    
    int (*mixer_alloc_substream)       (component_handle_t Component, int *SubStreamId);
    int (*mixer_free_substream)        (component_handle_t Component, int SubStreamId);
    int (*mixer_setup_substream)       (component_handle_t Component, int SubStreamId,
                                        struct alsa_substream_descriptor *Descriptor);
    int (*mixer_prepare_substream)     (component_handle_t Component, int SubStreamId);
    int (*mixer_start_substream)       (component_handle_t Component, int SubStreamId);
    int (*mixer_stop_substream)        (component_handle_t Component, int SubStreamId);
};

int register_alsa_backend       (char                           *name,
                                 struct alsa_backend_operations *alsa_backend_ops);
                                 
#endif /* H_ALSA_BACKEND_OPS */
