/*
   This file is part of GNU APL, a free implementation of the
   ISO/IEC Standard 13751, "Programming Language APL, Extended"
 
   Copyright (C) 2008-2015  Dr. Jürgen Sauermann
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
 
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


/********

Usage:

    m4 -D Svar_signals=def_file < tcp_signal.m4 > $@

This reads a Svar_signals specification from file def_file.def and prints
a header file on stdout that defines (the serialization of) signals according
to def_file.def.

In a Makefile, you would use it, for example, like this:

    my_signal.hh:  tcp_signal.m4 my_signal.def
            m4 -D Svar_signals=tcp_signal $< > $@

to produce my_signal.hh from my_signal.def. After that, my_signal.hh
can be used to send the signals defined in tcp_signal.def from one process
to another process.

That is:

                      my_signal.def
                            |
                            |
                            | tcp_signal.m4
                            |
                            V
                      my_signal.hh
                            |
                            |
                            |
                            |
                            V
                         my_client_program.cc
                         my_server_program.cc
                                   |
                                   |
                                   |    compile
                                   |    & link
                                   V
                                   my_client_program
                                   my_server_program


and then:

    my_client_program  ---  signals defined in ---> my_server_program
    my_server_program  ---  in my_signal.def   ---> my_client_program

********/

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <string>
#include <iostream>
#include <iomanip>

using namespace std;

//-----------------------------------------------------------------------------
/// an integer signal item of size \b bytes
template<typename T, int bytes>
class Sig_item_int
{
public:
   /// construct an item with value \b v
   Sig_item_int(T v) : value(v & get_mask()) {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_int(const uint8_t * & buffer)
      {
        value = 0;
        for (int b = 0; b < bytes; ++b)
            value = value << 8 | *buffer++;

        if (bytes == 6 && (value & 0x000080000000000ULL))
           value |= 0xFFFF000000000000ULL;
      }

   /// return the value of the item
   T get_value() const   { return value; }

   /// store (aka. serialize) this item into a string
   void store(string & buffer) const
      {
        for (int b = bytes; b > 0;)   buffer += char(value >> (8*--b));
      }

   /// print the item
   ostream & print(ostream & out) const
      {
        return out << value;
      }

   /// return the mask of the item, e.g. 0x00FFFFFF for a 3-byte integer
   static uint64_t get_mask()
      { uint64_t mask = 0;
        for (int b = 0; b < bytes; ++b)   mask = mask << 8 | 0x00FF;
        return mask;
      }

   /// the value of the item
   T value;
};
//-----------------------------------------------------------------------------
/// a hexadecimal integer signal item of size \b bytes
template<typename T, int bytes>
class Sig_item_xint : public Sig_item_int<T, bytes>
{
public:
   /// construct an item with value \b v
   Sig_item_xint(T v) : Sig_item_int<T, bytes>(v) {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_xint(const uint8_t * & buffer) : Sig_item_int<T, bytes>(buffer) {}

   /// print the item
   ostream & print(ostream & out) const
      {
        return out << "0x" << hex << setfill('0') << setw(bytes)
                   << Sig_item_int<T, bytes>::value
                   << setfill(' ') << dec;
      }
};
//-----------------------------------------------------------------------------
typedef Sig_item_int < int16_t, 1> Sig_item_i8;   ///<   8-bit signed integer
typedef Sig_item_int <uint16_t, 1> Sig_item_u8;   ///<   8-bit unsigned integer
typedef Sig_item_xint<uint16_t, 1> Sig_item_x8;   ///<   8-bit hex integer
typedef Sig_item_int < int16_t, 2> Sig_item_i16;   ///< 16-bit signed integer
typedef Sig_item_int <uint16_t, 2> Sig_item_u16;   ///< 16-bit unsigned integer
typedef Sig_item_xint<uint16_t, 2> Sig_item_x16;   ///< 16-bit hex integer
typedef Sig_item_int < int32_t, 3> Sig_item_i24;   ///< 24-bit signed integer
typedef Sig_item_int <uint32_t, 3> Sig_item_u24;   ///< 24-bit unsigned integer
typedef Sig_item_xint<uint32_t, 3> Sig_item_x24;   ///< 24-bit hex integer
typedef Sig_item_int < int32_t, 4> Sig_item_i32;   ///< 32-bit signed integer
typedef Sig_item_int <uint32_t, 4> Sig_item_u32;   ///< 32-bit unsigned integer
typedef Sig_item_xint<uint32_t, 4> Sig_item_x32;   ///< 32-bit hex integer
typedef Sig_item_int < int64_t, 6> Sig_item_i48;   ///< 48-bit signed integer
typedef Sig_item_int <uint64_t, 6> Sig_item_u48;   ///< 48-bit unsigned integer
typedef Sig_item_xint<uint64_t, 6> Sig_item_x48;   ///< 48-bit hex integer
typedef Sig_item_int < int64_t, 8> Sig_item_i64;   ///< 64-bit signed integer
typedef Sig_item_int <uint64_t, 8> Sig_item_u64;   ///< 64-bit unsigned integer
typedef Sig_item_xint<uint64_t, 8> Sig_item_x64;   ///< 64-bit hex integer

/// a string signal item of size \b bytes
class Sig_item_string
{
public:
   /// construct an item with value \b v
   Sig_item_string(const string & str)
   : value(str)
   {}

   /// construct (deserialize) this item from a (received) buffer
   Sig_item_string(const uint8_t * & buffer)
      {
        Sig_item_u16 len (buffer);
        value.reserve(len.get_value() + 2);
        for (int b = 0; b < len.get_value(); ++b)   value += *buffer++;
      }

   /// return the value of the item
   const string get_value() const   { return value; }

   /// store (aka. serialize) this item into a buffer
   void store(string & buffer) const
      {
        const Sig_item_u16 len (value.size());
        len.store(buffer);
        for (int b = 0; b < value.size(); ++b)   buffer += value[b];
      }

   /// print the item
   ostream & print(ostream & out) const
      {
        bool printable = true;
        for (int b = 0; b < value.size(); ++b)
            {
              const int v = value[b] & 0xFF;
              if (v < ' ' || v > '~')   { printable = false;   break; }
            }

        if (printable)   return out << "\"" << value << "\"";

        out << hex << setfill('0');
        for (int b = 0; b < value.size(); ++b)
            {
              if (b > 16)   { out << "...";   break; }
              out << " " << setw(2) << (value[b] & 0xFF);
            }
        return out << dec << setfill(' ');
      }

protected:
   /// the value of the item
   string value;
};
//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/// a number identifying the signal
enum Signal_id
{
/*
*/


/// ⎕SVO
   sid_MAKE_OFFER,
/// ⎕SVR
   sid_RETRACT_OFFER,
/// ⎕SVR
   sid_RETRACT_VAR,

/// set state of shared var \b key
   sid_SET_STATE,

/// set control of shared var \b key
   sid_SET_CONTROL,

/// X←SVAR
   sid_GET_VALUE,

/// X←SVAR result
   sid_VALUE_IS,

/// SVAR←X
   sid_ASSIGN_VALUE,
/// SVAR←X result
   sid_SVAR_ASSIGNED,

/// Can svar key be referenced ?
   sid_MAY_USE,

/// Can svar key be assigned ?
   sid_MAY_SET,

/// read SVAR database record from APserver
///
///		apl/APnnn	--> READ_SVAR_RECORD		APserver
///				<-- SVAR_RECORD_IS
///
   sid_READ_SVAR_RECORD,
/// result (record) for read SVAR database record from APserver
   sid_SVAR_RECORD_IS,

/// is ID registered ?
   sid_IS_REGISTERED_ID,

/// yes (1) or no (0)
   sid_YES_NO,

/// register processor proc in APserver
   sid_REGISTER_PROCESSOR,

/// match offered shared variable or create a new offer
   sid_MATCH_OR_MAKE,

/// result for match or create
   sid_MATCH_OR_MAKE_RESULT,


/// find processor ID that offers \b key
   sid_FIND_OFFERING_ID,

/// result of find processor ID that offers \b key
   sid_OFFERING_ID_IS,

/// get offering processors  (⎕SVQ)
   sid_GET_OFFERING_PROCS,

/// result of get offering processors  (⎕SVQ)
   sid_OFFERING_PROCS_ARE,

/// get offered variables  (⎕SVQ)
   sid_GET_OFFERED_VARS,

/// result of get offered variables  (⎕SVQ)
   sid_OFFERED_VARS_ARE,

/// find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
   sid_FIND_PAIRING_KEY,

/// result of find pairing key
   sid_PAIRING_KEY_IS,

/// result of find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
/// get events for one processor (first shared var with an event)
   sid_GET_EVENTS,

/// clear all events for one processor
   sid_CLEAR_ALL_EVENTS,


/// result of clear all events for one processor (first cleared svar
   sid_EVENTS_ARE,

/// add an event  for \b key
   sid_ADD_EVENT,

/// ws-ws SVAR←X
   sid_ASSIGN_WSWS_VAR,
/// X←ws-ws SVAR
   sid_READ_WSWS_VAR,
/// result of X←ws-ws SVAR
   sid_WSWS_VALUE_IS,

/// print the entire database (for command ]SVARS)
   sid_PRINT_SVAR_DB,
/// print the entire database result
   sid_SVAR_DB_PRINTED,

   sid_MAX,
};
//----------------------------------------------------------------------------
/// the base class for all signal classes
class Signal_base
{
public:
   /// destructor
   virtual ~Signal_base() {}

   /// store (encode) the signal into buffer
   virtual void store(string & buffer) const = 0;

   /// print the signal
   virtual ostream & print(ostream & out) const = 0;

   /// return the ID of the signal
   virtual Signal_id get_sigID() const = 0;

   /// return the name of the ID of the signal
   virtual const char * get_sigName() const = 0;

   /// get function for an item that is not defined for the signal
   void bad_get(const char * signal, const char * member) const
      {
        cerr << endl << "*** called function get_" << signal << "__" << member
             << "() with wrong signal " << get_sigName() << endl;
        assert(0 && "bad_get()");
      }

/*
*/


/// ⎕SVO
   /// access functions for signal MAKE_OFFER...
   virtual uint64_t get__MAKE_OFFER__key() const   ///< dito
      { bad_get("MAKE_OFFER", "key"); return 0; }

/// ⎕SVR
   /// access functions for signal RETRACT_OFFER...
   virtual uint64_t get__RETRACT_OFFER__key() const   ///< dito
      { bad_get("RETRACT_OFFER", "key"); return 0; }

/// ⎕SVR
   /// access functions for signal RETRACT_VAR...
   virtual uint64_t get__RETRACT_VAR__key() const   ///< dito
      { bad_get("RETRACT_VAR", "key"); return 0; }


/// set state of shared var \b key
   /// access functions for signal SET_STATE...
   virtual uint64_t get__SET_STATE__key() const   ///< dito
      { bad_get("SET_STATE", "key"); return 0; }
   virtual uint8_t get__SET_STATE__new_state() const   ///< dito
      { bad_get("SET_STATE", "new_state"); return 0; }
   virtual string get__SET_STATE__loc() const   ///< dito
      { bad_get("SET_STATE", "loc"); return 0; }


/// set control of shared var \b key
   /// access functions for signal SET_CONTROL...
   virtual uint64_t get__SET_CONTROL__key() const   ///< dito
      { bad_get("SET_CONTROL", "key"); return 0; }
   virtual uint8_t get__SET_CONTROL__new_control() const   ///< dito
      { bad_get("SET_CONTROL", "new_control"); return 0; }


/// X←SVAR
   /// access functions for signal GET_VALUE...
   virtual uint64_t get__GET_VALUE__key() const   ///< dito
      { bad_get("GET_VALUE", "key"); return 0; }


/// X←SVAR result
   /// access functions for signal VALUE_IS...
   virtual uint64_t get__VALUE_IS__key() const   ///< dito
      { bad_get("VALUE_IS", "key"); return 0; }
   virtual uint32_t get__VALUE_IS__error() const   ///< dito
      { bad_get("VALUE_IS", "error"); return 0; }
   virtual string get__VALUE_IS__error_loc() const   ///< dito
      { bad_get("VALUE_IS", "error_loc"); return 0; }
   virtual string get__VALUE_IS__cdr_value() const   ///< dito
      { bad_get("VALUE_IS", "cdr_value"); return 0; }


/// SVAR←X
   /// access functions for signal ASSIGN_VALUE...
   virtual uint64_t get__ASSIGN_VALUE__key() const   ///< dito
      { bad_get("ASSIGN_VALUE", "key"); return 0; }
   virtual string get__ASSIGN_VALUE__cdr_value() const   ///< dito
      { bad_get("ASSIGN_VALUE", "cdr_value"); return 0; }

/// SVAR←X result
   /// access functions for signal SVAR_ASSIGNED...
   virtual uint64_t get__SVAR_ASSIGNED__key() const   ///< dito
      { bad_get("SVAR_ASSIGNED", "key"); return 0; }
   virtual uint32_t get__SVAR_ASSIGNED__error() const   ///< dito
      { bad_get("SVAR_ASSIGNED", "error"); return 0; }
   virtual string get__SVAR_ASSIGNED__error_loc() const   ///< dito
      { bad_get("SVAR_ASSIGNED", "error_loc"); return 0; }


/// Can svar key be referenced ?
   /// access functions for signal MAY_USE...
   virtual uint64_t get__MAY_USE__key() const   ///< dito
      { bad_get("MAY_USE", "key"); return 0; }
   virtual int32_t get__MAY_USE__attempt() const   ///< dito
      { bad_get("MAY_USE", "attempt"); return 0; }


/// Can svar key be assigned ?
   /// access functions for signal MAY_SET...
   virtual uint64_t get__MAY_SET__key() const   ///< dito
      { bad_get("MAY_SET", "key"); return 0; }
   virtual int32_t get__MAY_SET__attempt() const   ///< dito
      { bad_get("MAY_SET", "attempt"); return 0; }


/// read SVAR database record from APserver
///
///		apl/APnnn	--> READ_SVAR_RECORD		APserver
///				<-- SVAR_RECORD_IS
///
   /// access functions for signal READ_SVAR_RECORD...
   virtual uint64_t get__READ_SVAR_RECORD__key() const   ///< dito
      { bad_get("READ_SVAR_RECORD", "key"); return 0; }

/// result (record) for read SVAR database record from APserver
   /// access functions for signal SVAR_RECORD_IS...
   virtual string get__SVAR_RECORD_IS__record() const   ///< dito
      { bad_get("SVAR_RECORD_IS", "record"); return 0; }


/// is ID registered ?
   /// access functions for signal IS_REGISTERED_ID...
   virtual uint32_t get__IS_REGISTERED_ID__proc() const   ///< dito
      { bad_get("IS_REGISTERED_ID", "proc"); return 0; }
   virtual uint32_t get__IS_REGISTERED_ID__parent() const   ///< dito
      { bad_get("IS_REGISTERED_ID", "parent"); return 0; }
   virtual uint32_t get__IS_REGISTERED_ID__grand() const   ///< dito
      { bad_get("IS_REGISTERED_ID", "grand"); return 0; }


/// yes (1) or no (0)
   /// access functions for signal YES_NO...
   virtual uint8_t get__YES_NO__yes() const   ///< dito
      { bad_get("YES_NO", "yes"); return 0; }


/// register processor proc in APserver
   /// access functions for signal REGISTER_PROCESSOR...
   virtual uint32_t get__REGISTER_PROCESSOR__proc() const   ///< dito
      { bad_get("REGISTER_PROCESSOR", "proc"); return 0; }
   virtual uint32_t get__REGISTER_PROCESSOR__parent() const   ///< dito
      { bad_get("REGISTER_PROCESSOR", "parent"); return 0; }
   virtual uint32_t get__REGISTER_PROCESSOR__grand() const   ///< dito
      { bad_get("REGISTER_PROCESSOR", "grand"); return 0; }
   virtual uint8_t get__REGISTER_PROCESSOR__evconn() const   ///< dito
      { bad_get("REGISTER_PROCESSOR", "evconn"); return 0; }
   virtual string get__REGISTER_PROCESSOR__progname() const   ///< dito
      { bad_get("REGISTER_PROCESSOR", "progname"); return 0; }


/// match offered shared variable or create a new offer
   /// access functions for signal MATCH_OR_MAKE...
   virtual string get__MATCH_OR_MAKE__varname() const   ///< dito
      { bad_get("MATCH_OR_MAKE", "varname"); return 0; }
   virtual uint32_t get__MATCH_OR_MAKE__to_proc() const   ///< dito
      { bad_get("MATCH_OR_MAKE", "to_proc"); return 0; }
   virtual uint32_t get__MATCH_OR_MAKE__to_parent() const   ///< dito
      { bad_get("MATCH_OR_MAKE", "to_parent"); return 0; }
   virtual uint32_t get__MATCH_OR_MAKE__to_grand() const   ///< dito
      { bad_get("MATCH_OR_MAKE", "to_grand"); return 0; }
   virtual uint32_t get__MATCH_OR_MAKE__from_proc() const   ///< dito
      { bad_get("MATCH_OR_MAKE", "from_proc"); return 0; }
   virtual uint32_t get__MATCH_OR_MAKE__from_parent() const   ///< dito
      { bad_get("MATCH_OR_MAKE", "from_parent"); return 0; }
   virtual uint32_t get__MATCH_OR_MAKE__from_grand() const   ///< dito
      { bad_get("MATCH_OR_MAKE", "from_grand"); return 0; }


/// result for match or create
   /// access functions for signal MATCH_OR_MAKE_RESULT...
   virtual uint64_t get__MATCH_OR_MAKE_RESULT__key() const   ///< dito
      { bad_get("MATCH_OR_MAKE_RESULT", "key"); return 0; }



/// find processor ID that offers \b key
   /// access functions for signal FIND_OFFERING_ID...
   virtual uint64_t get__FIND_OFFERING_ID__key() const   ///< dito
      { bad_get("FIND_OFFERING_ID", "key"); return 0; }


/// result of find processor ID that offers \b key
   /// access functions for signal OFFERING_ID_IS...
   virtual uint32_t get__OFFERING_ID_IS__proc() const   ///< dito
      { bad_get("OFFERING_ID_IS", "proc"); return 0; }
   virtual uint32_t get__OFFERING_ID_IS__parent() const   ///< dito
      { bad_get("OFFERING_ID_IS", "parent"); return 0; }
   virtual uint32_t get__OFFERING_ID_IS__grand() const   ///< dito
      { bad_get("OFFERING_ID_IS", "grand"); return 0; }


/// get offering processors  (⎕SVQ)
   /// access functions for signal GET_OFFERING_PROCS...
   virtual uint32_t get__GET_OFFERING_PROCS__offered_to_proc() const   ///< dito
      { bad_get("GET_OFFERING_PROCS", "offered_to_proc"); return 0; }


/// result of get offering processors  (⎕SVQ)
   /// access functions for signal OFFERING_PROCS_ARE...
   virtual string get__OFFERING_PROCS_ARE__offering_procs() const   ///< dito
      { bad_get("OFFERING_PROCS_ARE", "offering_procs"); return 0; }


/// get offered variables  (⎕SVQ)
   /// access functions for signal GET_OFFERED_VARS...
   virtual uint32_t get__GET_OFFERED_VARS__offered_to_proc() const   ///< dito
      { bad_get("GET_OFFERED_VARS", "offered_to_proc"); return 0; }
   virtual uint32_t get__GET_OFFERED_VARS__accepted_by_proc() const   ///< dito
      { bad_get("GET_OFFERED_VARS", "accepted_by_proc"); return 0; }


/// result of get offered variables  (⎕SVQ)
   /// access functions for signal OFFERED_VARS_ARE...
   virtual string get__OFFERED_VARS_ARE__offered_vars() const   ///< dito
      { bad_get("OFFERED_VARS_ARE", "offered_vars"); return 0; }


/// find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
   /// access functions for signal FIND_PAIRING_KEY...
   virtual uint64_t get__FIND_PAIRING_KEY__key() const   ///< dito
      { bad_get("FIND_PAIRING_KEY", "key"); return 0; }


/// result of find pairing key
   /// access functions for signal PAIRING_KEY_IS...
   virtual uint64_t get__PAIRING_KEY_IS__pairing_key() const   ///< dito
      { bad_get("PAIRING_KEY_IS", "pairing_key"); return 0; }


/// result of find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
/// get events for one processor (first shared var with an event)
   /// access functions for signal GET_EVENTS...
   virtual uint32_t get__GET_EVENTS__proc() const   ///< dito
      { bad_get("GET_EVENTS", "proc"); return 0; }
   virtual uint32_t get__GET_EVENTS__parent() const   ///< dito
      { bad_get("GET_EVENTS", "parent"); return 0; }
   virtual uint32_t get__GET_EVENTS__grand() const   ///< dito
      { bad_get("GET_EVENTS", "grand"); return 0; }


/// clear all events for one processor
   /// access functions for signal CLEAR_ALL_EVENTS...
   virtual uint32_t get__CLEAR_ALL_EVENTS__proc() const   ///< dito
      { bad_get("CLEAR_ALL_EVENTS", "proc"); return 0; }
   virtual uint32_t get__CLEAR_ALL_EVENTS__parent() const   ///< dito
      { bad_get("CLEAR_ALL_EVENTS", "parent"); return 0; }
   virtual uint32_t get__CLEAR_ALL_EVENTS__grand() const   ///< dito
      { bad_get("CLEAR_ALL_EVENTS", "grand"); return 0; }



/// result of clear all events for one processor (first cleared svar
   /// access functions for signal EVENTS_ARE...
   virtual uint64_t get__EVENTS_ARE__key() const   ///< dito
      { bad_get("EVENTS_ARE", "key"); return 0; }
   virtual uint32_t get__EVENTS_ARE__events() const   ///< dito
      { bad_get("EVENTS_ARE", "events"); return 0; }


/// add an event  for \b key
   /// access functions for signal ADD_EVENT...
   virtual uint64_t get__ADD_EVENT__key() const   ///< dito
      { bad_get("ADD_EVENT", "key"); return 0; }
   virtual uint32_t get__ADD_EVENT__proc() const   ///< dito
      { bad_get("ADD_EVENT", "proc"); return 0; }
   virtual uint32_t get__ADD_EVENT__parent() const   ///< dito
      { bad_get("ADD_EVENT", "parent"); return 0; }
   virtual uint32_t get__ADD_EVENT__grand() const   ///< dito
      { bad_get("ADD_EVENT", "grand"); return 0; }
   virtual uint32_t get__ADD_EVENT__event() const   ///< dito
      { bad_get("ADD_EVENT", "event"); return 0; }


/// ws-ws SVAR←X
   /// access functions for signal ASSIGN_WSWS_VAR...
   virtual uint64_t get__ASSIGN_WSWS_VAR__key() const   ///< dito
      { bad_get("ASSIGN_WSWS_VAR", "key"); return 0; }
   virtual string get__ASSIGN_WSWS_VAR__cdr_value() const   ///< dito
      { bad_get("ASSIGN_WSWS_VAR", "cdr_value"); return 0; }

/// X←ws-ws SVAR
   /// access functions for signal READ_WSWS_VAR...
   virtual uint64_t get__READ_WSWS_VAR__key() const   ///< dito
      { bad_get("READ_WSWS_VAR", "key"); return 0; }

/// result of X←ws-ws SVAR
   /// access functions for signal WSWS_VALUE_IS...
   virtual string get__WSWS_VALUE_IS__cdr_value() const   ///< dito
      { bad_get("WSWS_VALUE_IS", "cdr_value"); return 0; }


/// print the entire database (for command ]SVARS)
   /// access functions for signal PRINT_SVAR_DB...

/// print the entire database result
   /// access functions for signal SVAR_DB_PRINTED...
   virtual string get__SVAR_DB_PRINTED__printout() const   ///< dito
      { bad_get("SVAR_DB_PRINTED", "printout"); return 0; }



   /// receive a signal (TCP)
   inline static Signal_base * recv_TCP(int tcp_sock, char * buffer,
                                        int bufsize, char * & del,
                                        ostream * debug);

protected:

   /// send this signal on TCP (or AF_UNIX) socket tcp_sock
   int send_TCP(int tcp_sock) const
       {
         string buffer;
         store(buffer);

         char ll[sizeof(uint32_t)];
         *(uint32_t *)ll = htonl(buffer.size());
         send(tcp_sock, ll, 4, 0);
         ssize_t sent = send(tcp_sock, buffer.data(), buffer.size(), 0);
         return sent;
       }
};

/*
*/


/// ⎕SVO
//----------------------------------------------------------------------------
/// a class for MAKE_OFFER
class MAKE_OFFER_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   MAKE_OFFER_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   MAKE_OFFER_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_MAKE_OFFER);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "MAKE_OFFER(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_MAKE_OFFER; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "MAKE_OFFER"; }

  /// return item key of this signal 
   virtual uint64_t get__MAKE_OFFER__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};
/// ⎕SVR
//----------------------------------------------------------------------------
/// a class for RETRACT_OFFER
class RETRACT_OFFER_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   RETRACT_OFFER_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   RETRACT_OFFER_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_RETRACT_OFFER);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "RETRACT_OFFER(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_RETRACT_OFFER; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "RETRACT_OFFER"; }

  /// return item key of this signal 
   virtual uint64_t get__RETRACT_OFFER__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};
/// ⎕SVR
//----------------------------------------------------------------------------
/// a class for RETRACT_VAR
class RETRACT_VAR_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   RETRACT_VAR_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   RETRACT_VAR_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_RETRACT_VAR);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "RETRACT_VAR(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_RETRACT_VAR; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "RETRACT_VAR"; }

  /// return item key of this signal 
   virtual uint64_t get__RETRACT_VAR__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};

/// set state of shared var \b key
//----------------------------------------------------------------------------
/// a class for SET_STATE
class SET_STATE_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   SET_STATE_c(int s,
                Sig_item_x64 _key,
                Sig_item_u8 _new_state,
                Sig_item_string _loc)
   : key(_key),
     new_state(_new_state),
     loc(_loc)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   SET_STATE_c(const uint8_t * & buffer)
   : key(buffer),
     new_state(buffer),
     loc(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_SET_STATE);
         signal_id.store(buffer);
        key.store(buffer);
        new_state.store(buffer);
        loc.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "SET_STATE(";
        key.print(out);   out << ", ";
        new_state.print(out);   out << ", ";
        loc.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_SET_STATE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "SET_STATE"; }

  /// return item key of this signal 
   virtual uint64_t get__SET_STATE__key() const { return key.get_value(); }

  /// return item new_state of this signal 
   virtual uint8_t get__SET_STATE__new_state() const { return new_state.get_value(); }

  /// return item loc of this signal 
   virtual string get__SET_STATE__loc() const { return loc.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u8 new_state;   ///< new_state
   Sig_item_string loc;   ///< loc
};

/// set control of shared var \b key
//----------------------------------------------------------------------------
/// a class for SET_CONTROL
class SET_CONTROL_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   SET_CONTROL_c(int s,
                Sig_item_x64 _key,
                Sig_item_u8 _new_control)
   : key(_key),
     new_control(_new_control)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   SET_CONTROL_c(const uint8_t * & buffer)
   : key(buffer),
     new_control(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_SET_CONTROL);
         signal_id.store(buffer);
        key.store(buffer);
        new_control.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "SET_CONTROL(";
        key.print(out);   out << ", ";
        new_control.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_SET_CONTROL; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "SET_CONTROL"; }

  /// return item key of this signal 
   virtual uint64_t get__SET_CONTROL__key() const { return key.get_value(); }

  /// return item new_control of this signal 
   virtual uint8_t get__SET_CONTROL__new_control() const { return new_control.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u8 new_control;   ///< new_control
};

/// X←SVAR
//----------------------------------------------------------------------------
/// a class for GET_VALUE
class GET_VALUE_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   GET_VALUE_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   GET_VALUE_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_GET_VALUE);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "GET_VALUE(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_GET_VALUE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "GET_VALUE"; }

  /// return item key of this signal 
   virtual uint64_t get__GET_VALUE__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};

/// X←SVAR result
//----------------------------------------------------------------------------
/// a class for VALUE_IS
class VALUE_IS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   VALUE_IS_c(int s,
                Sig_item_x64 _key,
                Sig_item_u32 _error,
                Sig_item_string _error_loc,
                Sig_item_string _cdr_value)
   : key(_key),
     error(_error),
     error_loc(_error_loc),
     cdr_value(_cdr_value)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   VALUE_IS_c(const uint8_t * & buffer)
   : key(buffer),
     error(buffer),
     error_loc(buffer),
     cdr_value(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_VALUE_IS);
         signal_id.store(buffer);
        key.store(buffer);
        error.store(buffer);
        error_loc.store(buffer);
        cdr_value.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "VALUE_IS(";
        key.print(out);   out << ", ";
        error.print(out);   out << ", ";
        error_loc.print(out);   out << ", ";
        cdr_value.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_VALUE_IS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "VALUE_IS"; }

  /// return item key of this signal 
   virtual uint64_t get__VALUE_IS__key() const { return key.get_value(); }

  /// return item error of this signal 
   virtual uint32_t get__VALUE_IS__error() const { return error.get_value(); }

  /// return item error_loc of this signal 
   virtual string get__VALUE_IS__error_loc() const { return error_loc.get_value(); }

  /// return item cdr_value of this signal 
   virtual string get__VALUE_IS__cdr_value() const { return cdr_value.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u32 error;   ///< error
   Sig_item_string error_loc;   ///< error_loc
   Sig_item_string cdr_value;   ///< cdr_value
};

/// SVAR←X
//----------------------------------------------------------------------------
/// a class for ASSIGN_VALUE
class ASSIGN_VALUE_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   ASSIGN_VALUE_c(int s,
                Sig_item_x64 _key,
                Sig_item_string _cdr_value)
   : key(_key),
     cdr_value(_cdr_value)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   ASSIGN_VALUE_c(const uint8_t * & buffer)
   : key(buffer),
     cdr_value(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_ASSIGN_VALUE);
         signal_id.store(buffer);
        key.store(buffer);
        cdr_value.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "ASSIGN_VALUE(";
        key.print(out);   out << ", ";
        cdr_value.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_ASSIGN_VALUE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "ASSIGN_VALUE"; }

  /// return item key of this signal 
   virtual uint64_t get__ASSIGN_VALUE__key() const { return key.get_value(); }

  /// return item cdr_value of this signal 
   virtual string get__ASSIGN_VALUE__cdr_value() const { return cdr_value.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_string cdr_value;   ///< cdr_value
};
/// SVAR←X result
//----------------------------------------------------------------------------
/// a class for SVAR_ASSIGNED
class SVAR_ASSIGNED_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   SVAR_ASSIGNED_c(int s,
                Sig_item_x64 _key,
                Sig_item_u32 _error,
                Sig_item_string _error_loc)
   : key(_key),
     error(_error),
     error_loc(_error_loc)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   SVAR_ASSIGNED_c(const uint8_t * & buffer)
   : key(buffer),
     error(buffer),
     error_loc(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_SVAR_ASSIGNED);
         signal_id.store(buffer);
        key.store(buffer);
        error.store(buffer);
        error_loc.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "SVAR_ASSIGNED(";
        key.print(out);   out << ", ";
        error.print(out);   out << ", ";
        error_loc.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_SVAR_ASSIGNED; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "SVAR_ASSIGNED"; }

  /// return item key of this signal 
   virtual uint64_t get__SVAR_ASSIGNED__key() const { return key.get_value(); }

  /// return item error of this signal 
   virtual uint32_t get__SVAR_ASSIGNED__error() const { return error.get_value(); }

  /// return item error_loc of this signal 
   virtual string get__SVAR_ASSIGNED__error_loc() const { return error_loc.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u32 error;   ///< error
   Sig_item_string error_loc;   ///< error_loc
};

/// Can svar key be referenced ?
//----------------------------------------------------------------------------
/// a class for MAY_USE
class MAY_USE_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   MAY_USE_c(int s,
                Sig_item_x64 _key,
                Sig_item_i32 _attempt)
   : key(_key),
     attempt(_attempt)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   MAY_USE_c(const uint8_t * & buffer)
   : key(buffer),
     attempt(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_MAY_USE);
         signal_id.store(buffer);
        key.store(buffer);
        attempt.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "MAY_USE(";
        key.print(out);   out << ", ";
        attempt.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_MAY_USE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "MAY_USE"; }

  /// return item key of this signal 
   virtual uint64_t get__MAY_USE__key() const { return key.get_value(); }

  /// return item attempt of this signal 
   virtual int32_t get__MAY_USE__attempt() const { return attempt.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_i32 attempt;   ///< attempt
};

/// Can svar key be assigned ?
//----------------------------------------------------------------------------
/// a class for MAY_SET
class MAY_SET_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   MAY_SET_c(int s,
                Sig_item_x64 _key,
                Sig_item_i32 _attempt)
   : key(_key),
     attempt(_attempt)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   MAY_SET_c(const uint8_t * & buffer)
   : key(buffer),
     attempt(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_MAY_SET);
         signal_id.store(buffer);
        key.store(buffer);
        attempt.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "MAY_SET(";
        key.print(out);   out << ", ";
        attempt.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_MAY_SET; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "MAY_SET"; }

  /// return item key of this signal 
   virtual uint64_t get__MAY_SET__key() const { return key.get_value(); }

  /// return item attempt of this signal 
   virtual int32_t get__MAY_SET__attempt() const { return attempt.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_i32 attempt;   ///< attempt
};

/// read SVAR database record from APserver
///
///		apl/APnnn	--> READ_SVAR_RECORD		APserver
///				<-- SVAR_RECORD_IS
///
//----------------------------------------------------------------------------
/// a class for READ_SVAR_RECORD
class READ_SVAR_RECORD_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   READ_SVAR_RECORD_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   READ_SVAR_RECORD_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_READ_SVAR_RECORD);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "READ_SVAR_RECORD(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_READ_SVAR_RECORD; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "READ_SVAR_RECORD"; }

  /// return item key of this signal 
   virtual uint64_t get__READ_SVAR_RECORD__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};
/// result (record) for read SVAR database record from APserver
//----------------------------------------------------------------------------
/// a class for SVAR_RECORD_IS
class SVAR_RECORD_IS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   SVAR_RECORD_IS_c(int s,
                Sig_item_string _record)
   : record(_record)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   SVAR_RECORD_IS_c(const uint8_t * & buffer)
   : record(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_SVAR_RECORD_IS);
         signal_id.store(buffer);
        record.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "SVAR_RECORD_IS(";
        record.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_SVAR_RECORD_IS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "SVAR_RECORD_IS"; }

  /// return item record of this signal 
   virtual string get__SVAR_RECORD_IS__record() const { return record.get_value(); }


protected:
   Sig_item_string record;   ///< record
};

/// is ID registered ?
//----------------------------------------------------------------------------
/// a class for IS_REGISTERED_ID
class IS_REGISTERED_ID_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   IS_REGISTERED_ID_c(int s,
                Sig_item_u32 _proc,
                Sig_item_u32 _parent,
                Sig_item_u32 _grand)
   : proc(_proc),
     parent(_parent),
     grand(_grand)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   IS_REGISTERED_ID_c(const uint8_t * & buffer)
   : proc(buffer),
     parent(buffer),
     grand(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_IS_REGISTERED_ID);
         signal_id.store(buffer);
        proc.store(buffer);
        parent.store(buffer);
        grand.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "IS_REGISTERED_ID(";
        proc.print(out);   out << ", ";
        parent.print(out);   out << ", ";
        grand.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_IS_REGISTERED_ID; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "IS_REGISTERED_ID"; }

  /// return item proc of this signal 
   virtual uint32_t get__IS_REGISTERED_ID__proc() const { return proc.get_value(); }

  /// return item parent of this signal 
   virtual uint32_t get__IS_REGISTERED_ID__parent() const { return parent.get_value(); }

  /// return item grand of this signal 
   virtual uint32_t get__IS_REGISTERED_ID__grand() const { return grand.get_value(); }


protected:
   Sig_item_u32 proc;   ///< proc
   Sig_item_u32 parent;   ///< parent
   Sig_item_u32 grand;   ///< grand
};

/// yes (1) or no (0)
//----------------------------------------------------------------------------
/// a class for YES_NO
class YES_NO_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   YES_NO_c(int s,
                Sig_item_u8 _yes)
   : yes(_yes)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   YES_NO_c(const uint8_t * & buffer)
   : yes(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_YES_NO);
         signal_id.store(buffer);
        yes.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "YES_NO(";
        yes.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_YES_NO; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "YES_NO"; }

  /// return item yes of this signal 
   virtual uint8_t get__YES_NO__yes() const { return yes.get_value(); }


protected:
   Sig_item_u8 yes;   ///< yes
};

/// register processor proc in APserver
//----------------------------------------------------------------------------
/// a class for REGISTER_PROCESSOR
class REGISTER_PROCESSOR_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   REGISTER_PROCESSOR_c(int s,
                Sig_item_u32 _proc,
                Sig_item_u32 _parent,
                Sig_item_u32 _grand,
                Sig_item_u8 _evconn,
                Sig_item_string _progname)
   : proc(_proc),
     parent(_parent),
     grand(_grand),
     evconn(_evconn),
     progname(_progname)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   REGISTER_PROCESSOR_c(const uint8_t * & buffer)
   : proc(buffer),
     parent(buffer),
     grand(buffer),
     evconn(buffer),
     progname(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_REGISTER_PROCESSOR);
         signal_id.store(buffer);
        proc.store(buffer);
        parent.store(buffer);
        grand.store(buffer);
        evconn.store(buffer);
        progname.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "REGISTER_PROCESSOR(";
        proc.print(out);   out << ", ";
        parent.print(out);   out << ", ";
        grand.print(out);   out << ", ";
        evconn.print(out);   out << ", ";
        progname.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_REGISTER_PROCESSOR; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "REGISTER_PROCESSOR"; }

  /// return item proc of this signal 
   virtual uint32_t get__REGISTER_PROCESSOR__proc() const { return proc.get_value(); }

  /// return item parent of this signal 
   virtual uint32_t get__REGISTER_PROCESSOR__parent() const { return parent.get_value(); }

  /// return item grand of this signal 
   virtual uint32_t get__REGISTER_PROCESSOR__grand() const { return grand.get_value(); }

  /// return item evconn of this signal 
   virtual uint8_t get__REGISTER_PROCESSOR__evconn() const { return evconn.get_value(); }

  /// return item progname of this signal 
   virtual string get__REGISTER_PROCESSOR__progname() const { return progname.get_value(); }


protected:
   Sig_item_u32 proc;   ///< proc
   Sig_item_u32 parent;   ///< parent
   Sig_item_u32 grand;   ///< grand
   Sig_item_u8 evconn;   ///< evconn
   Sig_item_string progname;   ///< progname
};

/// match offered shared variable or create a new offer
//----------------------------------------------------------------------------
/// a class for MATCH_OR_MAKE
class MATCH_OR_MAKE_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   MATCH_OR_MAKE_c(int s,
                Sig_item_string _varname,
                Sig_item_u32 _to_proc,
                Sig_item_u32 _to_parent,
                Sig_item_u32 _to_grand,
                Sig_item_u32 _from_proc,
                Sig_item_u32 _from_parent,
                Sig_item_u32 _from_grand)
   : varname(_varname),
     to_proc(_to_proc),
     to_parent(_to_parent),
     to_grand(_to_grand),
     from_proc(_from_proc),
     from_parent(_from_parent),
     from_grand(_from_grand)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   MATCH_OR_MAKE_c(const uint8_t * & buffer)
   : varname(buffer),
     to_proc(buffer),
     to_parent(buffer),
     to_grand(buffer),
     from_proc(buffer),
     from_parent(buffer),
     from_grand(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_MATCH_OR_MAKE);
         signal_id.store(buffer);
        varname.store(buffer);
        to_proc.store(buffer);
        to_parent.store(buffer);
        to_grand.store(buffer);
        from_proc.store(buffer);
        from_parent.store(buffer);
        from_grand.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "MATCH_OR_MAKE(";
        varname.print(out);   out << ", ";
        to_proc.print(out);   out << ", ";
        to_parent.print(out);   out << ", ";
        to_grand.print(out);   out << ", ";
        from_proc.print(out);   out << ", ";
        from_parent.print(out);   out << ", ";
        from_grand.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_MATCH_OR_MAKE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "MATCH_OR_MAKE"; }

  /// return item varname of this signal 
   virtual string get__MATCH_OR_MAKE__varname() const { return varname.get_value(); }

  /// return item to_proc of this signal 
   virtual uint32_t get__MATCH_OR_MAKE__to_proc() const { return to_proc.get_value(); }

  /// return item to_parent of this signal 
   virtual uint32_t get__MATCH_OR_MAKE__to_parent() const { return to_parent.get_value(); }

  /// return item to_grand of this signal 
   virtual uint32_t get__MATCH_OR_MAKE__to_grand() const { return to_grand.get_value(); }

  /// return item from_proc of this signal 
   virtual uint32_t get__MATCH_OR_MAKE__from_proc() const { return from_proc.get_value(); }

  /// return item from_parent of this signal 
   virtual uint32_t get__MATCH_OR_MAKE__from_parent() const { return from_parent.get_value(); }

  /// return item from_grand of this signal 
   virtual uint32_t get__MATCH_OR_MAKE__from_grand() const { return from_grand.get_value(); }


protected:
   Sig_item_string varname;   ///< varname
   Sig_item_u32 to_proc;   ///< to_proc
   Sig_item_u32 to_parent;   ///< to_parent
   Sig_item_u32 to_grand;   ///< to_grand
   Sig_item_u32 from_proc;   ///< from_proc
   Sig_item_u32 from_parent;   ///< from_parent
   Sig_item_u32 from_grand;   ///< from_grand
};

/// result for match or create
//----------------------------------------------------------------------------
/// a class for MATCH_OR_MAKE_RESULT
class MATCH_OR_MAKE_RESULT_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   MATCH_OR_MAKE_RESULT_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   MATCH_OR_MAKE_RESULT_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_MATCH_OR_MAKE_RESULT);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "MATCH_OR_MAKE_RESULT(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_MATCH_OR_MAKE_RESULT; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "MATCH_OR_MAKE_RESULT"; }

  /// return item key of this signal 
   virtual uint64_t get__MATCH_OR_MAKE_RESULT__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};


/// find processor ID that offers \b key
//----------------------------------------------------------------------------
/// a class for FIND_OFFERING_ID
class FIND_OFFERING_ID_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   FIND_OFFERING_ID_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   FIND_OFFERING_ID_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_FIND_OFFERING_ID);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "FIND_OFFERING_ID(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_FIND_OFFERING_ID; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "FIND_OFFERING_ID"; }

  /// return item key of this signal 
   virtual uint64_t get__FIND_OFFERING_ID__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};

/// result of find processor ID that offers \b key
//----------------------------------------------------------------------------
/// a class for OFFERING_ID_IS
class OFFERING_ID_IS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   OFFERING_ID_IS_c(int s,
                Sig_item_u32 _proc,
                Sig_item_u32 _parent,
                Sig_item_u32 _grand)
   : proc(_proc),
     parent(_parent),
     grand(_grand)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   OFFERING_ID_IS_c(const uint8_t * & buffer)
   : proc(buffer),
     parent(buffer),
     grand(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_OFFERING_ID_IS);
         signal_id.store(buffer);
        proc.store(buffer);
        parent.store(buffer);
        grand.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "OFFERING_ID_IS(";
        proc.print(out);   out << ", ";
        parent.print(out);   out << ", ";
        grand.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_OFFERING_ID_IS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "OFFERING_ID_IS"; }

  /// return item proc of this signal 
   virtual uint32_t get__OFFERING_ID_IS__proc() const { return proc.get_value(); }

  /// return item parent of this signal 
   virtual uint32_t get__OFFERING_ID_IS__parent() const { return parent.get_value(); }

  /// return item grand of this signal 
   virtual uint32_t get__OFFERING_ID_IS__grand() const { return grand.get_value(); }


protected:
   Sig_item_u32 proc;   ///< proc
   Sig_item_u32 parent;   ///< parent
   Sig_item_u32 grand;   ///< grand
};

/// get offering processors  (⎕SVQ)
//----------------------------------------------------------------------------
/// a class for GET_OFFERING_PROCS
class GET_OFFERING_PROCS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   GET_OFFERING_PROCS_c(int s,
                Sig_item_u32 _offered_to_proc)
   : offered_to_proc(_offered_to_proc)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   GET_OFFERING_PROCS_c(const uint8_t * & buffer)
   : offered_to_proc(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_GET_OFFERING_PROCS);
         signal_id.store(buffer);
        offered_to_proc.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "GET_OFFERING_PROCS(";
        offered_to_proc.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_GET_OFFERING_PROCS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "GET_OFFERING_PROCS"; }

  /// return item offered_to_proc of this signal 
   virtual uint32_t get__GET_OFFERING_PROCS__offered_to_proc() const { return offered_to_proc.get_value(); }


protected:
   Sig_item_u32 offered_to_proc;   ///< offered_to_proc
};

/// result of get offering processors  (⎕SVQ)
//----------------------------------------------------------------------------
/// a class for OFFERING_PROCS_ARE
class OFFERING_PROCS_ARE_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   OFFERING_PROCS_ARE_c(int s,
                Sig_item_string _offering_procs)
   : offering_procs(_offering_procs)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   OFFERING_PROCS_ARE_c(const uint8_t * & buffer)
   : offering_procs(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_OFFERING_PROCS_ARE);
         signal_id.store(buffer);
        offering_procs.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "OFFERING_PROCS_ARE(";
        offering_procs.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_OFFERING_PROCS_ARE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "OFFERING_PROCS_ARE"; }

  /// return item offering_procs of this signal 
   virtual string get__OFFERING_PROCS_ARE__offering_procs() const { return offering_procs.get_value(); }


protected:
   Sig_item_string offering_procs;   ///< offering_procs
};

/// get offered variables  (⎕SVQ)
//----------------------------------------------------------------------------
/// a class for GET_OFFERED_VARS
class GET_OFFERED_VARS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   GET_OFFERED_VARS_c(int s,
                Sig_item_u32 _offered_to_proc,
                Sig_item_u32 _accepted_by_proc)
   : offered_to_proc(_offered_to_proc),
     accepted_by_proc(_accepted_by_proc)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   GET_OFFERED_VARS_c(const uint8_t * & buffer)
   : offered_to_proc(buffer),
     accepted_by_proc(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_GET_OFFERED_VARS);
         signal_id.store(buffer);
        offered_to_proc.store(buffer);
        accepted_by_proc.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "GET_OFFERED_VARS(";
        offered_to_proc.print(out);   out << ", ";
        accepted_by_proc.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_GET_OFFERED_VARS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "GET_OFFERED_VARS"; }

  /// return item offered_to_proc of this signal 
   virtual uint32_t get__GET_OFFERED_VARS__offered_to_proc() const { return offered_to_proc.get_value(); }

  /// return item accepted_by_proc of this signal 
   virtual uint32_t get__GET_OFFERED_VARS__accepted_by_proc() const { return accepted_by_proc.get_value(); }


protected:
   Sig_item_u32 offered_to_proc;   ///< offered_to_proc
   Sig_item_u32 accepted_by_proc;   ///< accepted_by_proc
};

/// result of get offered variables  (⎕SVQ)
//----------------------------------------------------------------------------
/// a class for OFFERED_VARS_ARE
class OFFERED_VARS_ARE_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   OFFERED_VARS_ARE_c(int s,
                Sig_item_string _offered_vars)
   : offered_vars(_offered_vars)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   OFFERED_VARS_ARE_c(const uint8_t * & buffer)
   : offered_vars(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_OFFERED_VARS_ARE);
         signal_id.store(buffer);
        offered_vars.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "OFFERED_VARS_ARE(";
        offered_vars.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_OFFERED_VARS_ARE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "OFFERED_VARS_ARE"; }

  /// return item offered_vars of this signal 
   virtual string get__OFFERED_VARS_ARE__offered_vars() const { return offered_vars.get_value(); }


protected:
   Sig_item_string offered_vars;   ///< offered_vars
};

/// find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
//----------------------------------------------------------------------------
/// a class for FIND_PAIRING_KEY
class FIND_PAIRING_KEY_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   FIND_PAIRING_KEY_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   FIND_PAIRING_KEY_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_FIND_PAIRING_KEY);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "FIND_PAIRING_KEY(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_FIND_PAIRING_KEY; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "FIND_PAIRING_KEY"; }

  /// return item key of this signal 
   virtual uint64_t get__FIND_PAIRING_KEY__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};

/// result of find pairing key
//----------------------------------------------------------------------------
/// a class for PAIRING_KEY_IS
class PAIRING_KEY_IS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   PAIRING_KEY_IS_c(int s,
                Sig_item_x64 _pairing_key)
   : pairing_key(_pairing_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   PAIRING_KEY_IS_c(const uint8_t * & buffer)
   : pairing_key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_PAIRING_KEY_IS);
         signal_id.store(buffer);
        pairing_key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "PAIRING_KEY_IS(";
        pairing_key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_PAIRING_KEY_IS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "PAIRING_KEY_IS"; }

  /// return item pairing_key of this signal 
   virtual uint64_t get__PAIRING_KEY_IS__pairing_key() const { return pairing_key.get_value(); }


protected:
   Sig_item_x64 pairing_key;   ///< pairing_key
};

/// result of find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
/// get events for one processor (first shared var with an event)
//----------------------------------------------------------------------------
/// a class for GET_EVENTS
class GET_EVENTS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   GET_EVENTS_c(int s,
                Sig_item_u32 _proc,
                Sig_item_u32 _parent,
                Sig_item_u32 _grand)
   : proc(_proc),
     parent(_parent),
     grand(_grand)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   GET_EVENTS_c(const uint8_t * & buffer)
   : proc(buffer),
     parent(buffer),
     grand(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_GET_EVENTS);
         signal_id.store(buffer);
        proc.store(buffer);
        parent.store(buffer);
        grand.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "GET_EVENTS(";
        proc.print(out);   out << ", ";
        parent.print(out);   out << ", ";
        grand.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_GET_EVENTS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "GET_EVENTS"; }

  /// return item proc of this signal 
   virtual uint32_t get__GET_EVENTS__proc() const { return proc.get_value(); }

  /// return item parent of this signal 
   virtual uint32_t get__GET_EVENTS__parent() const { return parent.get_value(); }

  /// return item grand of this signal 
   virtual uint32_t get__GET_EVENTS__grand() const { return grand.get_value(); }


protected:
   Sig_item_u32 proc;   ///< proc
   Sig_item_u32 parent;   ///< parent
   Sig_item_u32 grand;   ///< grand
};

/// clear all events for one processor
//----------------------------------------------------------------------------
/// a class for CLEAR_ALL_EVENTS
class CLEAR_ALL_EVENTS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   CLEAR_ALL_EVENTS_c(int s,
                Sig_item_u32 _proc,
                Sig_item_u32 _parent,
                Sig_item_u32 _grand)
   : proc(_proc),
     parent(_parent),
     grand(_grand)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   CLEAR_ALL_EVENTS_c(const uint8_t * & buffer)
   : proc(buffer),
     parent(buffer),
     grand(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_CLEAR_ALL_EVENTS);
         signal_id.store(buffer);
        proc.store(buffer);
        parent.store(buffer);
        grand.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "CLEAR_ALL_EVENTS(";
        proc.print(out);   out << ", ";
        parent.print(out);   out << ", ";
        grand.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_CLEAR_ALL_EVENTS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "CLEAR_ALL_EVENTS"; }

  /// return item proc of this signal 
   virtual uint32_t get__CLEAR_ALL_EVENTS__proc() const { return proc.get_value(); }

  /// return item parent of this signal 
   virtual uint32_t get__CLEAR_ALL_EVENTS__parent() const { return parent.get_value(); }

  /// return item grand of this signal 
   virtual uint32_t get__CLEAR_ALL_EVENTS__grand() const { return grand.get_value(); }


protected:
   Sig_item_u32 proc;   ///< proc
   Sig_item_u32 parent;   ///< parent
   Sig_item_u32 grand;   ///< grand
};


/// result of clear all events for one processor (first cleared svar
//----------------------------------------------------------------------------
/// a class for EVENTS_ARE
class EVENTS_ARE_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   EVENTS_ARE_c(int s,
                Sig_item_x64 _key,
                Sig_item_u32 _events)
   : key(_key),
     events(_events)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   EVENTS_ARE_c(const uint8_t * & buffer)
   : key(buffer),
     events(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_EVENTS_ARE);
         signal_id.store(buffer);
        key.store(buffer);
        events.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "EVENTS_ARE(";
        key.print(out);   out << ", ";
        events.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_EVENTS_ARE; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "EVENTS_ARE"; }

  /// return item key of this signal 
   virtual uint64_t get__EVENTS_ARE__key() const { return key.get_value(); }

  /// return item events of this signal 
   virtual uint32_t get__EVENTS_ARE__events() const { return events.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u32 events;   ///< events
};

/// add an event  for \b key
//----------------------------------------------------------------------------
/// a class for ADD_EVENT
class ADD_EVENT_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   ADD_EVENT_c(int s,
                Sig_item_x64 _key,
                Sig_item_u32 _proc,
                Sig_item_u32 _parent,
                Sig_item_u32 _grand,
                Sig_item_u32 _event)
   : key(_key),
     proc(_proc),
     parent(_parent),
     grand(_grand),
     event(_event)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   ADD_EVENT_c(const uint8_t * & buffer)
   : key(buffer),
     proc(buffer),
     parent(buffer),
     grand(buffer),
     event(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_ADD_EVENT);
         signal_id.store(buffer);
        key.store(buffer);
        proc.store(buffer);
        parent.store(buffer);
        grand.store(buffer);
        event.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "ADD_EVENT(";
        key.print(out);   out << ", ";
        proc.print(out);   out << ", ";
        parent.print(out);   out << ", ";
        grand.print(out);   out << ", ";
        event.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_ADD_EVENT; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "ADD_EVENT"; }

  /// return item key of this signal 
   virtual uint64_t get__ADD_EVENT__key() const { return key.get_value(); }

  /// return item proc of this signal 
   virtual uint32_t get__ADD_EVENT__proc() const { return proc.get_value(); }

  /// return item parent of this signal 
   virtual uint32_t get__ADD_EVENT__parent() const { return parent.get_value(); }

  /// return item grand of this signal 
   virtual uint32_t get__ADD_EVENT__grand() const { return grand.get_value(); }

  /// return item event of this signal 
   virtual uint32_t get__ADD_EVENT__event() const { return event.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_u32 proc;   ///< proc
   Sig_item_u32 parent;   ///< parent
   Sig_item_u32 grand;   ///< grand
   Sig_item_u32 event;   ///< event
};

/// ws-ws SVAR←X
//----------------------------------------------------------------------------
/// a class for ASSIGN_WSWS_VAR
class ASSIGN_WSWS_VAR_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   ASSIGN_WSWS_VAR_c(int s,
                Sig_item_x64 _key,
                Sig_item_string _cdr_value)
   : key(_key),
     cdr_value(_cdr_value)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   ASSIGN_WSWS_VAR_c(const uint8_t * & buffer)
   : key(buffer),
     cdr_value(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_ASSIGN_WSWS_VAR);
         signal_id.store(buffer);
        key.store(buffer);
        cdr_value.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "ASSIGN_WSWS_VAR(";
        key.print(out);   out << ", ";
        cdr_value.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_ASSIGN_WSWS_VAR; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "ASSIGN_WSWS_VAR"; }

  /// return item key of this signal 
   virtual uint64_t get__ASSIGN_WSWS_VAR__key() const { return key.get_value(); }

  /// return item cdr_value of this signal 
   virtual string get__ASSIGN_WSWS_VAR__cdr_value() const { return cdr_value.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
   Sig_item_string cdr_value;   ///< cdr_value
};
/// X←ws-ws SVAR
//----------------------------------------------------------------------------
/// a class for READ_WSWS_VAR
class READ_WSWS_VAR_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   READ_WSWS_VAR_c(int s,
                Sig_item_x64 _key)
   : key(_key)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   READ_WSWS_VAR_c(const uint8_t * & buffer)
   : key(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_READ_WSWS_VAR);
         signal_id.store(buffer);
        key.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "READ_WSWS_VAR(";
        key.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_READ_WSWS_VAR; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "READ_WSWS_VAR"; }

  /// return item key of this signal 
   virtual uint64_t get__READ_WSWS_VAR__key() const { return key.get_value(); }


protected:
   Sig_item_x64 key;   ///< key
};
/// result of X←ws-ws SVAR
//----------------------------------------------------------------------------
/// a class for WSWS_VALUE_IS
class WSWS_VALUE_IS_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   WSWS_VALUE_IS_c(int s,
                Sig_item_string _cdr_value)
   : cdr_value(_cdr_value)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   WSWS_VALUE_IS_c(const uint8_t * & buffer)
   : cdr_value(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_WSWS_VALUE_IS);
         signal_id.store(buffer);
        cdr_value.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "WSWS_VALUE_IS(";
        cdr_value.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_WSWS_VALUE_IS; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "WSWS_VALUE_IS"; }

  /// return item cdr_value of this signal 
   virtual string get__WSWS_VALUE_IS__cdr_value() const { return cdr_value.get_value(); }


protected:
   Sig_item_string cdr_value;   ///< cdr_value
};

/// print the entire database (for command ]SVARS)
//----------------------------------------------------------------------------
/// a class for PRINT_SVAR_DB
class PRINT_SVAR_DB_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   PRINT_SVAR_DB_c(int s)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   PRINT_SVAR_DB_c(const uint8_t * & buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_PRINT_SVAR_DB);
         signal_id.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "PRINT_SVAR_DB(";

        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_PRINT_SVAR_DB; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "PRINT_SVAR_DB"; }


protected:
};
/// print the entire database result
//----------------------------------------------------------------------------
/// a class for SVAR_DB_PRINTED
class SVAR_DB_PRINTED_c : public Signal_base
{
public:

   /// contructor that creates the signal and sends it on TCP socket s
   SVAR_DB_PRINTED_c(int s,
                Sig_item_string _printout)
   : printout(_printout)
   { send_TCP(s); }

   /// construct (deserialize) this item from a (received) buffer
   /// id has already been load()ed.
   SVAR_DB_PRINTED_c(const uint8_t * & buffer)
   : printout(buffer)
   {}

   /// store (aka. serialize) this signal into a buffer
   virtual void store(string & buffer) const
       {
         const Sig_item_u16 signal_id(sid_SVAR_DB_PRINTED);
         signal_id.store(buffer);
        printout.store(buffer);
       }

   /// print this signal on out.
   virtual ostream & print(ostream & out) const
      {
        out << "SVAR_DB_PRINTED(";
        printout.print(out);
        return out << ")" << endl;
      }

   /// a unique number for this signal
   virtual Signal_id get_sigID() const   { return sid_SVAR_DB_PRINTED; }

   /// the name of this signal
   virtual const char * get_sigName() const   { return "SVAR_DB_PRINTED"; }

  /// return item printout of this signal 
   virtual string get__SVAR_DB_PRINTED__printout() const { return printout.get_value(); }


protected:
   Sig_item_string printout;   ///< printout
};

//----------------------------------------------------------------------------

/// a union big enough for all signal classes
struct _all_signal_classes_
{

/*
*/


/// ⎕SVO
        char u_MAKE_OFFER[sizeof(MAKE_OFFER_c)];
/// ⎕SVR
        char u_RETRACT_OFFER[sizeof(RETRACT_OFFER_c)];
/// ⎕SVR
        char u_RETRACT_VAR[sizeof(RETRACT_VAR_c)];

/// set state of shared var \b key
        char u_SET_STATE[sizeof(SET_STATE_c)];

/// set control of shared var \b key
        char u_SET_CONTROL[sizeof(SET_CONTROL_c)];

/// X←SVAR
        char u_GET_VALUE[sizeof(GET_VALUE_c)];

/// X←SVAR result
        char u_VALUE_IS[sizeof(VALUE_IS_c)];

/// SVAR←X
        char u_ASSIGN_VALUE[sizeof(ASSIGN_VALUE_c)];
/// SVAR←X result
        char u_SVAR_ASSIGNED[sizeof(SVAR_ASSIGNED_c)];

/// Can svar key be referenced ?
        char u_MAY_USE[sizeof(MAY_USE_c)];

/// Can svar key be assigned ?
        char u_MAY_SET[sizeof(MAY_SET_c)];

/// read SVAR database record from APserver
///
///		apl/APnnn	--> READ_SVAR_RECORD		APserver
///				<-- SVAR_RECORD_IS
///
        char u_READ_SVAR_RECORD[sizeof(READ_SVAR_RECORD_c)];
/// result (record) for read SVAR database record from APserver
        char u_SVAR_RECORD_IS[sizeof(SVAR_RECORD_IS_c)];

/// is ID registered ?
        char u_IS_REGISTERED_ID[sizeof(IS_REGISTERED_ID_c)];

/// yes (1) or no (0)
        char u_YES_NO[sizeof(YES_NO_c)];

/// register processor proc in APserver
        char u_REGISTER_PROCESSOR[sizeof(REGISTER_PROCESSOR_c)];

/// match offered shared variable or create a new offer
        char u_MATCH_OR_MAKE[sizeof(MATCH_OR_MAKE_c)];

/// result for match or create
        char u_MATCH_OR_MAKE_RESULT[sizeof(MATCH_OR_MAKE_RESULT_c)];


/// find processor ID that offers \b key
        char u_FIND_OFFERING_ID[sizeof(FIND_OFFERING_ID_c)];

/// result of find processor ID that offers \b key
        char u_OFFERING_ID_IS[sizeof(OFFERING_ID_IS_c)];

/// get offering processors  (⎕SVQ)
        char u_GET_OFFERING_PROCS[sizeof(GET_OFFERING_PROCS_c)];

/// result of get offering processors  (⎕SVQ)
        char u_OFFERING_PROCS_ARE[sizeof(OFFERING_PROCS_ARE_c)];

/// get offered variables  (⎕SVQ)
        char u_GET_OFFERED_VARS[sizeof(GET_OFFERED_VARS_c)];

/// result of get offered variables  (⎕SVQ)
        char u_OFFERED_VARS_ARE[sizeof(OFFERED_VARS_ARE_c)];

/// find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
        char u_FIND_PAIRING_KEY[sizeof(FIND_PAIRING_KEY_c)];

/// result of find pairing key
        char u_PAIRING_KEY_IS[sizeof(PAIRING_KEY_IS_c)];

/// result of find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
/// get events for one processor (first shared var with an event)
        char u_GET_EVENTS[sizeof(GET_EVENTS_c)];

/// clear all events for one processor
        char u_CLEAR_ALL_EVENTS[sizeof(CLEAR_ALL_EVENTS_c)];


/// result of clear all events for one processor (first cleared svar
        char u_EVENTS_ARE[sizeof(EVENTS_ARE_c)];

/// add an event  for \b key
        char u_ADD_EVENT[sizeof(ADD_EVENT_c)];

/// ws-ws SVAR←X
        char u_ASSIGN_WSWS_VAR[sizeof(ASSIGN_WSWS_VAR_c)];
/// X←ws-ws SVAR
        char u_READ_WSWS_VAR[sizeof(READ_WSWS_VAR_c)];
/// result of X←ws-ws SVAR
        char u_WSWS_VALUE_IS[sizeof(WSWS_VALUE_IS_c)];

/// print the entire database (for command ]SVARS)
        char u_PRINT_SVAR_DB[sizeof(PRINT_SVAR_DB_c)];
/// print the entire database result
        char u_SVAR_DB_PRINTED[sizeof(SVAR_DB_PRINTED_c)];

};

enum { MAX_SIGNAL_CLASS_SIZE = sizeof(_all_signal_classes_) };

//----------------------------------------------------------------------------
Signal_base *
Signal_base::recv_TCP(int tcp_sock, char * buffer, int bufsize,
                                 char * & del, ostream * debug)
{
   if (bufsize < 2*MAX_SIGNAL_CLASS_SIZE)
      {
         // a too small bufsize happens easily but is hard to debug!
         //
         cerr << "\n\n*** bufsize is " << bufsize
              << " but MUST be at least " << 2*MAX_SIGNAL_CLASS_SIZE
              << " in recv_TCP() !!!" << endl;

         return 0;
      }
uint32_t siglen = 0;
   for (;;)
       {
         errno = 0;
         const ssize_t rx_bytes = ::recv(tcp_sock, buffer,
                                     sizeof(uint32_t), MSG_WAITALL);

         if (errno == EINTR)   continue;

         if (rx_bytes != sizeof(uint32_t))
            {
                 // connection was closed by the peer
                 return 0;
            }

         break;   // got  sizeof(uint32_t) length bytes
       }
//    debug && *debug << "rx_bytes is " << rx_bytes
//                    << " when reading siglen in in recv_TCP()" << endl;

   siglen = ntohl(*(uint32_t *)buffer);
   if (siglen == 0)   return 0;   // close

// debug && *debug << "signal length is " << siglen << " in recv_TCP()" << endl;

   // skip MAX_SIGNAL_CLASS_SIZE bytes at the beginning of buffer
   //
char * rx_buf = buffer + MAX_SIGNAL_CLASS_SIZE;
   bufsize -= MAX_SIGNAL_CLASS_SIZE;

   if (siglen > bufsize)
      {
        // the buffer provided is too small: allocate a bigger one
        //
        del = new char[siglen];
        if (del == 0)
           {
             cerr << "*** new(" << siglen <<") failed in recv_TCP()" << endl;
             return 0;
           }
        rx_buf = del;
      }

   for (;;)
       {
          errno = 0;
          const ssize_t rx_bytes = ::recv(tcp_sock, rx_buf,
                                            siglen, MSG_WAITALL);

          if (errno == EINTR)   continue;

          if (rx_bytes != siglen)
             {
               cerr << "*** got " << rx_bytes
                    << " when expecting " << siglen << endl;
               return 0;
             }


         break;   // got siglen bytes
       }

// debug && *debug << "rx_bytes is " << rx_bytes << " in recv_TCP()" << endl;

const uint8_t * b = (const uint8_t *)rx_buf;
Sig_item_u16 signal_id(b);

Signal_base * ret = 0;
   switch(signal_id.get_value())
      {

/*
*/


/// ⎕SVO
        case sid_MAKE_OFFER: ret = new MAKE_OFFER_c(b);   break;
/// ⎕SVR
        case sid_RETRACT_OFFER: ret = new RETRACT_OFFER_c(b);   break;
/// ⎕SVR
        case sid_RETRACT_VAR: ret = new RETRACT_VAR_c(b);   break;

/// set state of shared var \b key
        case sid_SET_STATE: ret = new SET_STATE_c(b);   break;

/// set control of shared var \b key
        case sid_SET_CONTROL: ret = new SET_CONTROL_c(b);   break;

/// X←SVAR
        case sid_GET_VALUE: ret = new GET_VALUE_c(b);   break;

/// X←SVAR result
        case sid_VALUE_IS: ret = new VALUE_IS_c(b);   break;

/// SVAR←X
        case sid_ASSIGN_VALUE: ret = new ASSIGN_VALUE_c(b);   break;
/// SVAR←X result
        case sid_SVAR_ASSIGNED: ret = new SVAR_ASSIGNED_c(b);   break;

/// Can svar key be referenced ?
        case sid_MAY_USE: ret = new MAY_USE_c(b);   break;

/// Can svar key be assigned ?
        case sid_MAY_SET: ret = new MAY_SET_c(b);   break;

/// read SVAR database record from APserver
///
///		apl/APnnn	--> READ_SVAR_RECORD		APserver
///				<-- SVAR_RECORD_IS
///
        case sid_READ_SVAR_RECORD: ret = new READ_SVAR_RECORD_c(b);   break;
/// result (record) for read SVAR database record from APserver
        case sid_SVAR_RECORD_IS: ret = new SVAR_RECORD_IS_c(b);   break;

/// is ID registered ?
        case sid_IS_REGISTERED_ID: ret = new IS_REGISTERED_ID_c(b);   break;

/// yes (1) or no (0)
        case sid_YES_NO: ret = new YES_NO_c(b);   break;

/// register processor proc in APserver
        case sid_REGISTER_PROCESSOR: ret = new REGISTER_PROCESSOR_c(b);   break;

/// match offered shared variable or create a new offer
        case sid_MATCH_OR_MAKE: ret = new MATCH_OR_MAKE_c(b);   break;

/// result for match or create
        case sid_MATCH_OR_MAKE_RESULT: ret = new MATCH_OR_MAKE_RESULT_c(b);   break;


/// find processor ID that offers \b key
        case sid_FIND_OFFERING_ID: ret = new FIND_OFFERING_ID_c(b);   break;

/// result of find processor ID that offers \b key
        case sid_OFFERING_ID_IS: ret = new OFFERING_ID_IS_c(b);   break;

/// get offering processors  (⎕SVQ)
        case sid_GET_OFFERING_PROCS: ret = new GET_OFFERING_PROCS_c(b);   break;

/// result of get offering processors  (⎕SVQ)
        case sid_OFFERING_PROCS_ARE: ret = new OFFERING_PROCS_ARE_c(b);   break;

/// get offered variables  (⎕SVQ)
        case sid_GET_OFFERED_VARS: ret = new GET_OFFERED_VARS_c(b);   break;

/// result of get offered variables  (⎕SVQ)
        case sid_OFFERED_VARS_ARE: ret = new OFFERED_VARS_ARE_c(b);   break;

/// find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
        case sid_FIND_PAIRING_KEY: ret = new FIND_PAIRING_KEY_c(b);   break;

/// result of find pairing key
        case sid_PAIRING_KEY_IS: ret = new PAIRING_KEY_IS_c(b);   break;

/// result of find pairing key (CTL vs. DAT or Cnnn vs. Dnnn) for AP210
/// get events for one processor (first shared var with an event)
        case sid_GET_EVENTS: ret = new GET_EVENTS_c(b);   break;

/// clear all events for one processor
        case sid_CLEAR_ALL_EVENTS: ret = new CLEAR_ALL_EVENTS_c(b);   break;


/// result of clear all events for one processor (first cleared svar
        case sid_EVENTS_ARE: ret = new EVENTS_ARE_c(b);   break;

/// add an event  for \b key
        case sid_ADD_EVENT: ret = new ADD_EVENT_c(b);   break;

/// ws-ws SVAR←X
        case sid_ASSIGN_WSWS_VAR: ret = new ASSIGN_WSWS_VAR_c(b);   break;
/// X←ws-ws SVAR
        case sid_READ_WSWS_VAR: ret = new READ_WSWS_VAR_c(b);   break;
/// result of X←ws-ws SVAR
        case sid_WSWS_VALUE_IS: ret = new WSWS_VALUE_IS_c(b);   break;

/// print the entire database (for command ]SVARS)
        case sid_PRINT_SVAR_DB: ret = new PRINT_SVAR_DB_c(b);   break;
/// print the entire database result
        case sid_SVAR_DB_PRINTED: ret = new SVAR_DB_PRINTED_c(b);   break;

        default: cerr << "Signal_base::recv_TCP() failed: unknown signal id "
                      << signal_id.get_value() << endl;
                 errno = EINVAL;
                 return 0;
      }

   debug && ret->print(*debug << "<-- ");

   return ret;   // invalid id
}
//----------------------------------------------------------------------------
