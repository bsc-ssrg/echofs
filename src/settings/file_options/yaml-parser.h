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
 * This file contains the code to implement a YAML configuration file
 * parser, that reads the file structure from a file_schema instance
 * and returns an options_map with the parsed values as a result
 */

#include "file-schema.h"
#include "options-map.h"

namespace file_options {
namespace detail {

/*! This class implements a visitor that processes a 'file_schema' instance
 * and parses a corresponding YAML-formatted file according to the options
 * described in that schema. */
class yaml_visitor : public boost::static_visitor<> {
public:

    /*! Constructor. */
    yaml_visitor(const std::string& name, const YAML::Node& config, options_map& opt_map)
        : m_name(name),
          m_config(config),
          m_options_map(opt_map) { }

    /*! Process a 'group_schema'.
     * A 'group_schema' is a named section in a 'file_schema' that 
     * describes a related group of options:
     *
     *  "named-group": [
     *      opt0: "foo",
     *      opt1: "42"
     *  ]
     *
     * As such, a 'group_schema' represents a YAML sequence of entries,
     * where each entry corresponds to a map of key-value pairs
     */
    void operator()(group_schema& opt_group) const {

        std::set<std::string> mandatory_options;
        std::map<std::string, boost::any> default_values;

        // store any mandatory options so that we can check later if
        // the user provided them
        for(const auto& opt : opt_group) {

            const auto& opt_name = opt.first;
            const auto& opt_schema = opt.second;

            if(opt_schema.is_mandatory()) {
                mandatory_options.emplace(opt_name);
            }

            if(opt_schema.has_default_value()) {
                default_values.emplace(opt_name, opt_schema.default_value());
            }
        }

        options_group parsed_values;

        for(const auto& entry : m_config) {
            for(const auto& kv : entry) {
                const auto key = kv.first.as<std::string>();
                const auto text_value = kv.second.as<std::string>();

                auto it = opt_group.find(key);

                if(it == opt_group.end()) {
                    throw std::invalid_argument("Invalid argument '" + key + "' in '" + m_name + "'");
                }

                auto& opt_schema = it->second;

                boost::any value;

                if(opt_schema.has_converter()) {
                    value = opt_schema.convert(key, text_value);
                }
                else {
                    value = text_value;
                }

                parsed_values.emplace(key, option_value(value));
            }
        }

        // check whether all mandatory options were provided
        for(const auto& opt_name : mandatory_options) {
            if(parsed_values.count(opt_name) == 0) {
                throw std::invalid_argument("Missing mandatory option '" + opt_name + "'");
            }
        }

        // non-mandatory options not-provided by the user need to be 
        // initialized to their default values (if any)
        for(const auto& v : default_values) {

            const auto& opt_name = v.first;
            const auto& opt_value = v.second;

            if(parsed_values.count(opt_name) == 0) {
                parsed_values.emplace(opt_name, opt_value);
            }
        }

        m_options_map.emplace(m_name, std::move(parsed_values));
    }

    /*! Process a 'list_schema'.
     * A 'list_schema' is a named section in a 'file_schema' that 
     * describes a list of repeated group of options, as follows:
     *
     *  "named-list": [
     *      [ opt0: "foo",
     *        opt1: "42",
     *        opt3: "baz" ],
     *
     *      [ opt0: "bar",
     *        opt1: "6" ]
     *  ]
     *
     * As such, a 'list_schema' represents a YAML sequence of entries,
     * where each entry corresponds to another sequence where 
     * each of its entries is a map of key-value pairs
     */
    void operator()(list_schema& opt_list) const {
        (void) opt_list;

        bool accept_all = (opt_list.count(match_any) != 0);

        options_list parsed_values;

        for(const auto& entry : m_config) {

            options_group parsed_group;

            for(const auto& subentry : entry) {
                for(const auto& kv : subentry) {
                    const auto key = kv.first.as<std::string>();
                    const auto text_value = kv.second.as<std::string>();

                    auto it = opt_list.find(key);

                    if(it == opt_list.end()) {
                        if(!accept_all) {
                            throw std::invalid_argument("Invalid argument '" + key + "' in '" + m_name + "'");
                        }

                        it = opt_list.find(match_any);

                        assert(it != opt_list.end());
                    }

                    auto& opt_schema = it->second;

                    boost::any value;

                    if(opt_schema.has_converter()) {
                        value = opt_schema.convert(key, text_value);
                    }
                    else {
                        value = text_value;
                    }

                    parsed_group.emplace(key, option_value(value));
                }
            }

            parsed_values.emplace_back(parsed_group);
        }

        m_options_map.emplace(m_name, std::move(parsed_values));
    }

    const std::string  m_name;
    const YAML::Node&  m_config;
    options_map&       m_options_map;

};

} // namespace detail

/*! Parses 'yaml_file' and stores in 'opt_map' the values of any options defined in 'schema' */
void parse_yaml_file(const bfs::path& yaml_file, const file_schema& schema, options_map& opt_map) {

    const YAML::Node config = YAML::LoadFile(yaml_file.string());

    for(const auto& opt : schema) {

        auto opt_name = opt.first;
        auto opt_schema = opt.second;

        if(!config[opt_name]) {
            if(opt_schema.is_mandatory()) {
                throw std::invalid_argument("Missing mandatory '" + opt_name + "' block from configuration file");
            }
            continue;
        }

        boost::apply_visitor(
                detail::yaml_visitor(opt_name, config[opt_name], opt_map), 
                opt_schema.m_schema);
    }
}

} // namespace file_options
