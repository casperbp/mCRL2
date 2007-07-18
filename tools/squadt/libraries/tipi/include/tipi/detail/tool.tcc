//  Copyright 2007 Jeroen van der Wulp. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
/// \file include/tipi/detail/tool.tcc

#include "tipi/common.hpp"
#include <tipi/detail/basic_messenger.tcc>
#include "tipi/tool.hpp"
#include "tipi/tool/capabilities.hpp"
#include "tipi/layout_base.hpp"
#include "tipi/detail/message.hpp"
#include "tipi/detail/command_line_interface.hpp"
#include "tipi/visitors.hpp"

namespace tipi {
  namespace tool {

    class communicator_impl : public tipi::messaging::basic_messenger_impl< tipi::message > {
      friend class communicator;
      friend class messaging::scheme< tipi::message >;
 
      private:

        /** \brief The last received set of controller capabilities */
        boost::shared_ptr < controller::capabilities > current_controller_capabilities;
 
        /** \brief The object that descibed the capabilities of the current tool */
        tool::capabilities                             current_tool_capabilities;
 
        /** \brief This object reflects the current configuration */
        boost::shared_ptr < configuration >            current_configuration;
 
        /** \brief Unique identifier for the running tool, obtained via the command line */
        long                                           instance_identifier;
 
      private:

        /** \brief Send details about the amount of space that the controller currently has reserved for this tool */
        void request_tool_capabilities_handler();

        /** \brief Handler for incoming data resulting from user interaction with the display relayed by the controller */
        void receive_display_data_handler(const tipi::messenger::message_ptr&, layout::tool_display::sptr);

        /** \brief Extract a configuration from an offer_configuration message */
        void receive_configuration_handler(const tipi::messenger::message_ptr& m);

      public:
 
        /** \brief Constructor */
        communicator_impl();

        /** \brief Constructor that takes controller connection arguments from the command line */
        bool activate(tipi::tool::communicator*, int&, char** const);

        /** \brief Constructor that takes controller connection arguments from the command line */
        bool activate(tipi::tool::communicator*, char*&);

        /** \brief Attempts to build a connection using a scheme object */
        bool activate(tipi::tool::communicator*, command_line_interface::scheme_ptr const&, long const&);

        /** \brief Send a layout specification for the display space reserved for this tool */
        void send_display_layout(layout::tool_display::sptr);
 
        /** \brief Sends the empty layout specification for the display space */
        void send_clear_display();
    };

    /**
     * \attention please use connect() to manually establish a connection with a controller
     **/
    inline communicator_impl::communicator_impl() : current_tool_capabilities() {
 
      /* Register event handlers for some message types */
      add_handler(tipi::message_tool_capabilities, boost::bind(&communicator_impl::request_tool_capabilities_handler, this));
      add_handler(tipi::message_configuration, boost::bind(&communicator_impl::receive_configuration_handler, this, _1));
    }
 
    /**
     * \param[in,out] argv a pointer to the list of command line arguments
     **/
    inline bool communicator_impl::activate(tipi::tool::communicator* c, char*& argv) {
      command_line_interface::argument_extractor e(argv);

      return (activate(c, e.get_scheme(), e.get_identifier())); 
    }

    /**
     * \param[in,out] argc the number of command line arguments
     * \param[in,out] argv a pointer to the list of command line arguments
     **/
    inline bool communicator_impl::activate(tipi::tool::communicator* c, int& argc, char** const argv) {
      command_line_interface::argument_extractor e(argc, argv);

      return (activate(c, e.get_scheme(), e.get_identifier())); 
    }

    /**
     * \param[in] s a scheme object that represents the method of connecting
     * \param[in] id an identifier used in the connection
     **/
    inline bool communicator_impl::activate(tipi::tool::communicator* c, command_line_interface::scheme_ptr const& s, long const& id) {
      if (s.get() != 0) {
        s->connect(c);
 
        instance_identifier = id;
 
        /* Identify the tool instance to the controller */
        tipi::message m(boost::str(boost::format("%u") % id), tipi::message_instance_identification);
 
        send_message(m);
      }
 
      return (s.get() != 0);
    }

    inline void communicator_impl::send_display_layout(layout::tool_display::sptr d) {
      tipi::message m(tipi::visitors::store(*d), tipi::message_display_layout);

      send_message(m);

      clear_handlers(tipi::message_display_update);

      add_handler(tipi::message_display_update, boost::bind(&communicator_impl::receive_display_data_handler, this, _1, d));
    }

    inline void communicator_impl::send_clear_display() {
      layout::tool_display display;

      clear_handlers(tipi::message_display_update);

      tipi::message m(tipi::visitors::store(display), tipi::message_display_layout);

      send_message(m);
    }

    /* Send a specification of the tools capabilities */
    inline void communicator_impl::request_tool_capabilities_handler() {
      tipi::message m(tipi::visitors::store(current_tool_capabilities), tipi::message_tool_capabilities);
 
      send_message(m);
    }
 
    /**
     * \param[in] m shared pointer to the message
     * \param[out] d tool display on which to execute changes
     **/
    inline void communicator_impl::receive_display_data_handler(const tipi::messenger::message_ptr& m, layout::tool_display::sptr d) {
      std::vector < tipi::layout::element const* > elements;

      d->update(m->to_string(), elements);
    }
    
    /**
     * \param[in] m shared pointer reference to an offer_configuration message
     **/
    inline void communicator_impl::receive_configuration_handler(const tipi::messenger::message_ptr& m) {
      assert(m->get_type() == tipi::message_configuration);

      if (m.get() != 0) {
        boost::shared_ptr < configuration > c(new configuration);

        tipi::visitors::restore(*c, m->to_string());

        current_configuration = c;
      }
    }
  } 
}
