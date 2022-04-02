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


#ifndef INCLUDED_DSMX_PREAMBLEDETECTION_H
#define INCLUDED_DSMX_PREAMBLEDETECTION_H

#include <gnuradio/dsmx/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace dsmx {

    /*!
     * \brief <+description of block+>
     * \ingroup dsmx
     *
     */
    class DSMX_API preambleDetection : virtual public gr::sync_block
    {
     public:
      typedef std::shared_ptr<preambleDetection> sptr;

      /*!
       * \brief Return a shared_ptr to a new instance of dsmx::preambleDetection.
       *
       * To avoid accidental use of raw pointers, dsmx::preambleDetection's
       * constructor is in a private implementation
       * class. dsmx::preambleDetection::make is the public interface for
       * creating new instances.
       */
      static sptr make(int channel);

      virtual void set_new_channel(int new_channel) = 0;
    };

  } // namespace dsmx
} // namespace gr

#endif /* INCLUDED_DSMX_PREAMBLEDETECTION_H */
