/*
 *  Driver for the TKDMA
 *
 * Copyright (C) 2007 STMicroelectronics Limited
 * Author: Peter Bennett <peter.bennett@st.com>
 *
 * May be copied or modified under the terms of the GNU General Public
 * License.  See linux/COPYING for more information.
 */

#ifndef __TKDMA_H
#define __TKDMA_H

#define TKDMA_VIDEO_CHANNEL 0
#define TKDMA_AUDIO_CHANNEL 1

int  tkdma_bluray_decrypt_data(char *dest, char *src,int num_packets,int device);
int  tkdma_hddvd_decrypt_data(char *dest, char *src,int num_packets,int channel,int device);
void tkdma_set_iv(char *iv);
void tkdma_set_key(char *key,int slot);
void tkdma_set_cpikey(char *key,int slot);

#endif
