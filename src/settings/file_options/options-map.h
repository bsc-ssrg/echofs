/*************************************************************************
 * (C) Copyright 2016-2017 Barcelona Supercomputing Center               *
 *                         Centro Nacional de Supercomputacion           *
 *                                                                       *
 * This file is part of the Echo Filesystem NG.                          *
 *                                                                       *
 * See AUTHORS file in the top level directory for information           *
 * regarding developers and contributors.                                *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU Lesser General Public            *
 * License as published by the Free Software Foundation; either          *
 * version 3 of the License, or (at your option) any later version.      *
 *                                                                       *
 * The Echo Filesystem NG is distributed in the hope that it will        *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied         *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR               *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Echo Filesystem NG; if not, write to the Free      *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    *
 *                                                                       *
 *************************************************************************/

 /*
* This software was developed as part of the
* EC H2020 funded project NEXTGenIO (Project ID: 671951)
* www.nextgenio.eu
*/ 


/*
 * This file contains the code to implement an options map that can
 * be generated by parsing a configuration according to the structure
 * defined by a file_schema instance.
 */

#ifndef __OPTIONS_MAP_H__
#define __OPTIONS_MAP_H__

#include <yaml-cpp/yaml.h>
#include <boost/any.hpp>

namespace file_options {

/*! Class to handle values of an arbitrary type */
struct option_value {
    /*! Constructor. The 'value' parameter is  passed (and stored)
     * as boost::any in order to handle arbitrarily-typed values */
    option_value(const boost::any& value)
        : m_value(value) {}

    /*! If the stored value is of type T, returns that value.
     * Otherwise throws boost::bad_any_cast exception */
    template <typename T>
    T& get_as() {
        return boost::any_cast<T&>(m_value);
    }

    /*! If the stored value is of type T, returns that value.
     * Otherwise throws boost::bad_any_cast exception */
    template <typename T>
    const T& get_as() const {
        return boost::any_cast<const T&>(m_value);
    }

    /*! Stored option value */
    boost::any m_value;
};

/*! Class to handle a group of related options. The stored options
 * can be accessed using the usual std::map interface. */
struct options_group : public std::map<std::string, option_value> {

    /*! If the options_group contains an option named 'opt_name'
     * of type T, returns the value for that option. 
     * If the option does not exist, throws std::invalid_argument exception.
     * Otherwise throws boost::bad_any_cast exception */
    template <typename T>
    const T& get_as(const std::string& opt_name) const {

        auto it = find(opt_name);

        if(it == end()) {
            throw std::invalid_argument("option '" + opt_name + "' missing");
        }

        return it->second.get_as<T>();
    }
};

/*! Class to handle a list of several groups */
using options_list = std::vector<options_group>;

/*! Class that stores the values for the defined variables so that they
 * can be accessed in a key-value manner.
 * This class is derived from std::map<std::string, boost::any>, 
 * therefore all usual std::map operators can be used to access
 * its contents */
struct options_map : public std::map<std::string, boost::any> {

    /*! If the options_map contains an option named 'opt_name'
     * of type T, returns the value for that option. 
     * If the option does not exist, throws std::invalid_argument exception.
     * Otherwise throws boost::bad_any_cast exception */
    template <typename T>
    T& get_as(std::string opt_name) {

        auto it = find(opt_name);

        if(it == end()) {
            throw std::invalid_argument("option '" + opt_name + "' missing");
        }

        return boost::any_cast<T&>(it->second);
    }
};

} // namespace file_options

#endif /* __OPTIONS_MAP_H__ */
