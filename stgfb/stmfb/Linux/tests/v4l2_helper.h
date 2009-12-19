#ifndef __V4L2_HELPER__
#define __V4L2_HELPER__

#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

#define NOCOL      "\033[0m"

#define BLACKCOL   "\033[0;30m"
#define LGRAYCOL   "\033[0;37m"

#define REDCOL     "\033[0;31m"          // dvb warning
#define GREENCOL   "\033[0;32m"          // ui message
#define BROWNCOL   "\033[0;33m"
#define BLUECOL    "\033[0;34m"          // dvb message
#define PURPLECOL  "\033[0;35m"          // ui warning
#define CYANCOL    "\033[0;36m"

#define DGRAYCOL   "\033[1;30m"          // ca message
#define WHITECOL   "\033[1;37m"          // ca warning

#define LREDCOL     "\033[1;31m"
#define LGREENCOL   "\033[1;32m"         // dfb message
#define YELLOWCOL   "\033[1;33m"         // gfx print
#define LBLUECOL    "\033[1;34m"         // osd layer print
#define LPURPLECOL  "\033[1;35m"         // dfb warning
#define LCYANCOL    "\033[1;36m"         // scl layer print


#define ltv_print(format, args...) \
  ( { printf ("(%.5d) " __FILE__ ": " format, getpid (), ## args); } )
#define ltv_message(format, args...) \
  ( { printf (BLUECOL "(%.5d) message: " __FILE__ "(%d): " format NOCOL "\n", getpid (), __LINE__ , ## args); } )
#define ltv_warning(format, args...) \
  ( { printf (REDCOL "(%.5d) warning: " __FILE__ "(%d): " format NOCOL "\n", getpid (), __LINE__, ## args); } )


#define LTV_CHECK(x...)                                               \
  ( {                                                                 \
    int ret = x;                                                      \
    if (ret < 0)                                                      \
      {                                                               \
        typeof(errno) errno_backup = errno;                           \
        ltv_warning ("%s: %s: %d (%m)", __FUNCTION__, #x, errno);     \
        errno = errno_backup;                                         \
      }                                                               \
    ret;                                                              \
  } )



static void
__attribute__((unused))
v4l2_list_outputs (int videofd)
{
  struct v4l2_output output;

  output.index = 0;
  while (ioctl (videofd, VIDIOC_ENUMOUTPUT, &output) == 0)
    {
      ltv_message ("videofd %d: Found output '%s' w/ idx/std/type/audioset/modulator: %d/%.16llx/%d/%d/%d",
		   videofd,
		   output.name, output.index, output.std, output.type,
		   output.audioset, output.modulator);
      ++output.index;
    }
}

static int
v4l2_set_output_by_index (int videofd,
			  int output_idx)
{
  int output_idx_read;
  int ret = -1;
  typeof (errno) errno_backup;

  if (output_idx < 0)
    /* invalid output */
    return ret;

  ltv_print ("Setting output to %u\n", output_idx);
  ret = LTV_CHECK (ioctl (videofd, VIDIOC_S_OUTPUT, &output_idx));
  errno_backup = errno;
  LTV_CHECK (ioctl (videofd, VIDIOC_G_OUTPUT, &output_idx_read));
  if (output_idx != output_idx_read)
    ltv_warning ("Driver BUG: output index is %d (should be: %d) - ignoring\n",
		 output_idx_read, output_idx);

  errno = errno_backup;
  return ret;
}

static int
__attribute__((unused))
v4l2_set_output_by_name (int         videofd,
			 const char * const name)
{
  struct v4l2_output output;
  int                output_idx;

  output.index = 0;
  output_idx   = -1;
  while (ioctl (videofd, VIDIOC_ENUMOUTPUT, &output) == 0)
    {
      if (strcasecmp ((char *) output.name, name) == 0) {
        output_idx = output.index;
	break;
      }

      ++output.index;
    }

  return v4l2_set_output_by_index (videofd, output_idx);
}


#endif /* __V4L2_HELPER__ */
