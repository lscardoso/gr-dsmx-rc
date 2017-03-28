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
#include "preambleDetection_impl.h"
#include <stdio.h>
#include <stdlib.h>


namespace gr {
  namespace dsmx {

    preambleDetection::sptr
    preambleDetection::make(int channel)
    {
      return gnuradio::get_initial_sptr
        (new preambleDetection_impl(channel));
    }

    /*
     * The private constructor
     */
    preambleDetection_impl::preambleDetection_impl(int channel)
      : gr::sync_block("preambleDetection",
              gr::io_signature::make(1, 1, sizeof(char)),
              gr::io_signature::make(0, 0, sizeof(char)))
    {
      d_not_Found = 0;
      preambleDetection_impl::set_new_channel(channel);
      d_samples_processed = 2000;
      message_port_register_out(PDU_PORT_ID);
      d_type = gr::blocks::pdu::byte_t;
      d_pdu_meta = pmt::PMT_NIL;
      d_pdu_vector = pmt::PMT_NIL;
      d_state = 0;
      d_nbr_offsets = 30 ;
      d_sopBuffer.resize(d_nbr_offsets + 1, std::vector<uint8_t>(128/8, 0));
      d_error_threshold = 5;
    }

    /*
     * Our virtual destructor.
     */
    preambleDetection_impl::~preambleDetection_impl()
    {
    }

    unsigned char reverse(unsigned char b) {
      b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
      b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
      b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
      return b;
    }

    int distance(uint8_t a, uint8_t b){
      int distance = 0;
      for (size_t i = 0; i < 8; i++) {
        if (((a >> i)&1) != ((b >> i)&1) ){
          distance++;
        }
      }
      return distance;
    }


    int
    preambleDetection_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const char *in = (const char *) input_items[0];
      char *out = (char *) output_items[0];
      std::vector<tag_t> tags;
      preambleDetection_impl::get_tags_in_window(tags, 0, 0, noutput_items, pmt::mp("PreambleFound"));


      int indexInTagList = 0;
      int itemBaseIndex = preambleDetection_impl::nitems_read(0);
      for(int i = 0; i < noutput_items; i++){
        if(d_samples_processed < d_nbr_samples_to_process + d_nbr_offsets){

          switch (d_state) {
            case 0: //Find SOP
              for(int offset = 0; offset< d_sopBuffer.size(); offset++){

                if (d_samples_processed >= offset && d_samples_processed-offset <64) {
                  d_sopBuffer[offset][(d_samples_processed-offset)/8] = (d_sopBuffer[offset][(d_samples_processed-offset)/8] << 1) | in[i];
                }else if ((d_samples_processed - offset) == 64) {
                  for(int loop = 0; loop < 8; loop++){
                    sopRecieved[loop] = reverse(d_sopBuffer[offset][7-loop]); //Reverse byte order and bid order inside bytes
                  }
                  for(int x = 0; x < 9; x++){ //We already know which row we are in since we know the current channel number
                    int error = 0;
                    for(int loop = 0; loop < 8; loop++){
                      error+= distance(sopRecieved[loop],pncodes[d_row][x][7-loop]);
                    }
                    if( error <= d_error_threshold){    //Good pn code Found, we now know what column is used and what offset is good to decode
                      d_column = x;
                      d_offset = offset;
                      d_state++;
                      break;
                    }else{
                      d_not_Found++;
                    }
                  }
                }
              }
              if (d_not_Found >= 5*9*d_sopBuffer.size()){ //If nothing was found we don't want to try to decode data
                d_state=10;
                d_not_Found = 0;
              }
              break;
            case 1: //Check correct data rate ( find NOT(SOP) )

              if (d_samples_processed - d_offset > 64 + 4) { //Saut de 4 bits
                d_sopBuffer[d_offset][(d_samples_processed - d_offset-4)/8] = (d_sopBuffer[d_offset][(d_samples_processed - d_offset-4)/8] << 1) | in[i];
                if (d_samples_processed - d_offset == 64 + 4 + 64) {
                  for(int loop = 0; loop < 8; loop++){
                    sopRecieved[loop] = reverse(~d_sopBuffer[d_offset][8 + 7-loop]); //NOT le PN code
                  }
                  int error = 0;
                  for(int loop = 0; loop < 8; loop++){
                    error += distance(sopRecieved[loop],pncodes[d_row][d_column][7-loop]);
                  }
                  if( error >= d_error_threshold){ //Too many bit errors
                    d_state = 10;
                  }
                  d_state++;
                }
              }
              break;

            case 2:
              if ( (d_samples_processed - d_offset > d_sop_part_length) && (d_samples_processed - d_offset < d_nbr_samples_to_process)) { //Saut de 1 bits
                    //One index of the buffer holds 8 input items (one byte) and 16 have already been processed in the sop code
                d_dataBuffer[(d_samples_processed - d_offset-5)/8 - 16] = (d_dataBuffer[(d_samples_processed - d_offset-5)/8 - 16] << 1) | in[i];
              }
              if((d_samples_processed - d_offset == d_nbr_samples_to_process-1)) {

                // Grab data, throw into vector
                d_pdu_vector = gr::blocks::pdu::make_pdu_vector(d_type, d_dataBuffer, (1+16+2)*8);
                d_pdu_meta = pmt::make_dict();
                d_pdu_meta = dict_add(d_pdu_meta, pmt::intern("Column"), pmt::mp(d_column));
                d_pdu_meta = dict_add(d_pdu_meta, pmt::intern("Row"), pmt::mp(d_row));
                d_pdu_meta = dict_add(d_pdu_meta, pmt::intern("Channel"), pmt::mp(d_channel));

                // Send msg
                pmt::pmt_t msg = pmt::cons(d_pdu_meta, d_pdu_vector);
                message_port_pub(PDU_PORT_ID, msg);

              }
              break;
          }
          d_samples_processed++;
        }
        else if(tags.size()>0 && itemBaseIndex+i == tags[indexInTagList].offset){   //Packet processed
          d_samples_processed=0;
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
      //return samples_processed + (indexInTagList*d_nbr_samples_to_process);
      return noutput_items;
    }

  } /* namespace dsmx */
} /* namespace gr */
