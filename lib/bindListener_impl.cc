/* -*- c++ -*- */
/*
 * Copyright 2017 <+YOU OR YOUR COMPANY+>.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "bindListener_impl.h"

namespace gr {
  namespace dsmx {

    bindListener::sptr
    bindListener::make()
    {
      return gnuradio::get_initial_sptr
        (new bindListener_impl());
    }

    /*
     * The private constructor
     */
    bindListener_impl::bindListener_impl()
      : gr::sync_block("bindListener",
              gr::io_signature::make(1, 1, sizeof(char)),
              gr::io_signature::make(0, 0, 0))
    {
      d_state = 0;
      d_nbr_offsets = 25 ;
      d_samples_processed = 30000;
      d_buffer.resize(d_nbr_offsets + 1, std::vector<uint64_t>((16*8), 0));
      d_buffer32.resize(d_nbr_offsets + 1, std::vector<uint32_t>((16*8*2), 0));
      d_data_chunks.resize(d_nbr_offsets + 1, std::vector<uint8_t>(16, 0));
      d_calls_offset0 =0;
    }

    /*
     * Our virtual destructor.
     */
    bindListener_impl::~bindListener_impl()
    {
    }

    /*
    * Decodes 1 bit of data using a known pn code. 1 if the code has been sent as is, 0 if it has had a NOT on it
    */
    int decodeBit(uint64_t data, uint64_t code, unsigned char threshold){
      int count=0 ;
      for(int i = 0; i<64; i++){
        if( ((code >> (i))& 1) == ((data >> i)& 1) ){
          count++;
        }
      }
      if( count <= threshold){    // So 64-threshold bits of error max. Every single bit is in error when you compare a code with its NOT version.
        return 0;
      }
      if( count >= (64 - threshold)){
        return 1;
      }
      return -1;
    }


    int
    bindListener_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const char *in = (const char *) input_items[0];

      std::vector<tag_t> tags;
      bindListener_impl::get_tags_in_window(tags, 0, 0, noutput_items, pmt::mp("PreambleFound"));

      int indexInTagList = 0;
      int itemBaseIndex = bindListener_impl::nitems_read(0);
      for(int i = 0; i < noutput_items; i++){
        if(d_samples_processed < d_nbr_samples_to_process + d_nbr_offsets){
          switch (d_state) {
            case 0: //Find Beginning of packet
              for(int offset = 0; offset< d_buffer.size(); offset++){
                if (d_samples_processed >= offset ) { //We fill the data in 2 32 bits buffers because a 64 bits buffer didn't want to fill up
                  d_buffer32[offset][(d_samples_processed-offset)/32] = (d_buffer32[offset][(d_samples_processed-offset)/32] << 1) | in[i];
                }
                if ((d_samples_processed - offset)% 64 == 63) { //When we have recieved our 64 bits, we treat it
                  d_buffer[offset][(d_samples_processed-offset)/64] = (uint64_t(d_buffer32[offset][((d_samples_processed-offset)/32) -1]) <<32) | uint64_t(d_buffer32[offset][((d_samples_processed-offset)/32)]);
                  int decoded = -1;
                  int x,y;
                  for(y = 0; y< 5;y++){ //Check over the 5*9 pn codes possible
                    for(x = 0; x < 9; x++){
                      decoded = decodeBit(d_buffer[offset][(d_samples_processed-offset)/64], Despreader_impl::cast864reverse(pncodes[y][x]), 5);
                      if (decoded !=-1) {
                        //printf("Offset %d using %dth row and %dth column decodes: \n",offset, y, x, decoded);
                        break;  //We found what we were looking for
                      }
                    }
                    if (decoded !=-1) {
                      break;    //To exit the second loop
                    }
                  }
                  if( decoded != -1){
                    d_not_Found = 0;
                    //Bytes are sent least-significant bit first and stored most-significant bit last so we need to fill the data bytes from the "top"
                    d_data_chunks[offset][((d_samples_processed - offset -1) / (8*64))] = (d_data_chunks[offset][((d_samples_processed - offset -1) / (8*64))] >> 1) | (decoded<<7);
                    //break;
                  }
                  else{
                    d_not_Found++;
                  }
                }
              }
              if (d_not_Found == 2*d_buffer.size()){
                d_state++;
                d_not_Found = 0;
                //printf("%s\n", "Nothing found");
              }
              break;
            case 1:
              break;

          }
          d_samples_processed++;
        }
        else if(tags.size()>0 && itemBaseIndex+i == tags[indexInTagList].offset){   //Packet processed
          d_samples_processed=0;

          if (d_state==0) {
            printf("%s\n", "Binding Packet recieved!");
            for (size_t offset = 0; offset < d_data_chunks.size(); offset++) {
              printf("\nOffset: %d ", offset);
              for (size_t index = 0; index < 16; index++) {
                printf(" %02X", d_data_chunks[offset][index]);
              }
            }
            printf("\n%s\n", "********************************************************");
          }
          d_state = 0;
          d_not_Found = 0;
        }
        if(tags.size()>0 && itemBaseIndex+i >= tags[indexInTagList].offset){    //Next packet to process
          if(indexInTagList < tags.size()-1){
            indexInTagList++;
          }
        }
      }

      tags.clear();

      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace dsmx */
} /* namespace gr */
