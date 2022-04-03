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
    preambleDetection::make()
    {
      return gnuradio::get_initial_sptr
        (new preambleDetection_impl());
    }

    /*
     * The private constructor
     */
    preambleDetection_impl::preambleDetection_impl()
      : gr::sync_block("preambleDetection",
              gr::io_signature::make(1, 1, sizeof(char)),
              gr::io_signature::make(1, 1, sizeof(char)))
    {
      d_samples_processed = 2000;
      message_port_register_out(pmt::mp("pdus"));
      d_type = gr::types::byte_t;
      d_pdu_meta = pmt::PMT_NIL;
      d_pdu_vector = pmt::PMT_NIL;
      d_state = 0;
      int numOffsets = 10;
      d_sopBuffer.resize(numOffsets + 1, std::vector<uint8_t>(128/8, 0));
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




    int
    preambleDetection_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const char *in = (const char *) input_items[0];
      char *out = (char *) output_items[0];
      //printf("%s\n", "Truc  called!");
      std::vector<tag_t> tags;
      preambleDetection_impl::get_tags_in_window(tags, 0, 0, noutput_items, pmt::mp("PreambleFound"));
      if(tags.size() != 0)
        printf("%d\n", tags.size());
      for(int i=0; i< tags.size();i++){
         std::cout << "Tag: " << tags[i].key << tags[i].value << std::endl;
      }
      d_pdu_meta = pmt::make_dict();
      int indexInTagList = 0;
      int itemBaseIndex = preambleDetection_impl::nitems_read(0);
      for(int i = 0; i < noutput_items; i++){
        out[i] = 0;
        if(d_samples_processed < d_nbr_samples_to_process){
          out[i] = in[i];

          switch (d_state) {
            case 0: //Find SOP
              for(d_offset = 0; d_offset< d_sopBuffer.size(); d_offset++){
                if (d_samples_processed >= d_offset ) {
                  d_sopBuffer[d_offset][(d_samples_processed-d_offset)/8] = (d_sopBuffer[d_offset][(d_samples_processed-d_offset)/8] << 1) | in[i];
                }
                if (d_samples_processed - d_offset == 64) {
                  for(int loop = 0; loop < 8; loop++){
                    sopRecieved[loop] = reverse(d_sopBuffer[d_offset][7-loop]);
                  }
                  for(int y = 0; y< 5;y++){
                    for(int x = 0; x < 9; x++){
                      int similarity = 0;
                      for(int loop = 0; loop < 8; loop++){
                        //if(sopRecieved[loop] != pncodes[d_row][d_column][loop]){
                        similarity+= std::abs(sopRecieved[loop]-pncodes[y][x][loop]);
                        //}
                      }
                      int similarityReverse = 0;
                      for(int loop = 0; loop < 8; loop++){
                        //if(sopRecieved[loop] == pncodes[d_row][d_column][7-loop]){
                        similarityReverse+= std::abs(sopRecieved[loop]-pncodes[y][x][7-loop]);
                        //}
                      }
                      if( similarity <= 10 || similarityReverse <= 10){
                        d_column = x;
                        d_row = y;
                        printf("Offset %d %s", d_offset, "Recieved : ");
                        for(int loop = 0; loop < 8; loop++){
                          printf("%02X ", sopRecieved[loop]);
                        }
                        printf("Similarity : %d,  SimilarityReverse : %d\n", similarity, similarityReverse);
                        printf("%s", "Matched  : ");
                        for(int loop = 0; loop < 8; loop++){
                          printf("%02X ", pncodes[d_row][d_column][loop]);
                        }
                        printf(" d_row: %d, column: %d\n", d_row,d_column);
                        d_state++;
                        break;
                      }else{
                        //printf("%s\n", "No match found.");
                        d_not_Found++;
                      }
                    }
                  }
                  //printf("Next state: %d\n", d_state);
                }
              }
              if (d_samples_processed - d_offset+1 == 64){
                d_state++;
              }
              break;
            case 1: //Check correct data rate (find NOT(SOP)
              if (d_samples_processed - d_offset > 64 + 4) { //Saut de 4 bits
                d_sopBuffer[d_offset][(d_samples_processed - d_offset-4)/8] = (d_sopBuffer[d_offset][(d_samples_processed - d_offset-4)/8] << 1) | in[i];
                if (d_samples_processed - d_offset == 64 + 4 + 64) {
                  //printf("%s\n", "Test");
                  for(int loop = 0; loop < 8; loop++){
                    sopRecieved[loop] = reverse(~d_sopBuffer[d_offset][8 + 7-loop]); //NOT le PN code
                  }
                  int similarity = 0;
                  for(int loop = 0; loop < 8; loop++){
                      similarity+= std::abs(sopRecieved[loop]-pncodes[d_row][d_column][loop]);
                  }
                  int similarityReverse = 0;
                  for(int loop = 0; loop < 8; loop++){
                      similarityReverse+= std::abs(sopRecieved[loop]-pncodes[d_row][d_column][7-loop]);
                  }
                  if( similarity <= 10 || similarityReverse<= 10){
                    /*printf("%s", "NOTRecieved : ");
                    for(int loop = 0; loop < 8; loop++){
                      printf("%02X ", sopRecieved[loop]);
                    }
                    printf("Similarity : %d,  SimilarityReverse : %d\n", similarity, similarityReverse);
                    printf("%s", "NOTMatched  : ");
                    for(int loop = 0; loop < 8; loop++){
                      printf("%02X ", pncodes[d_row][d_column][loop]);
                    }
                    printf(" d_row: %d, column: %d\n", d_row,d_column);*/
                  }else{
                    //printf("%s\n", "No match found.");
                    d_state = 10;
                  }

                  d_state++;
                }
              }
              break;

            case 2:
              if ( (d_samples_processed - d_offset > 64 + 4 + 64 + 1) && (d_samples_processed - d_offset < 64 + 4 + 64 + 1 + 64 + 16*64 + 2*64)) { //Saut de 1 bits
                d_dataBuffer[(d_samples_processed - d_offset-5)/8 - 16] = (d_dataBuffer[(d_samples_processed - d_offset-5)/8 - 16] << 1) | in[i];
              }
              break;
          }
          d_samples_processed++;
        }
        else if(tags.size()>0 && itemBaseIndex+i == tags[indexInTagList].offset){
          d_samples_processed=0;

          if (d_state==2) {
            printf("Samples to process: %d\n", d_nbr_samples_to_process);
            // Grab data, throw into vector
            d_pdu_vector = gr::pdu::make_pdu_vector(d_type, d_dataBuffer, (1+16 + 2)*8);
            d_pdu_meta = pmt::make_dict();
            d_pdu_meta = dict_add(d_pdu_meta, pmt::intern("Column"), pmt::mp(d_column));
            d_pdu_meta = dict_add(d_pdu_meta, pmt::intern("Row"), pmt::mp(d_row));

            // Send msg
            pmt::pmt_t msg = pmt::cons(d_pdu_meta, d_pdu_vector);
            message_port_pub(pmt::mp("pdus"), msg);
            printf("%s\n", "Published!");
          }
          d_state = 0;
        }
        if(tags.size()>0 && itemBaseIndex+i >= tags[indexInTagList].offset){
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
