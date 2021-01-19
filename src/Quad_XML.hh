/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2020  Dr. Jürgen Sauermann

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

#ifndef __Quad_XML_DEFINED__
#define __Quad_XML_DEFINED__

#include "QuadFunction.hh"

//-----------------------------------------------------------------------------
/// a helper class (doubly linked list) for parsing XML
class XML_node
{
public:
   /// constructor
   XML_node(XML_node * anchor, const UCS_string & src,
            ShapeItem spos, ShapeItem slen);

   /// destructor
   ~XML_node();

   /// the type of an XML_node
   enum Node_type
      {
        NT_error       = -1,
        NT_text        =  1,   // unstructured text (between the tags)
        NT_leaf_tag    =  2,   // <Tag ... />
        NT_start_tag   =  3,   // <Tag ... >
        NT_end_tag     =  4,    // </Tag>
        NT_comment     =  5,    // <!-- ... -->
        NT_declaration =  6,    // <?xml ... ?>
        NT_doctype     =  7,    // <!DOCTYPE ... >
      };

   /// return the next XML node in the document
   const XML_node * get_next() const
      { return next; }

   /// return the next XML node in the document
   XML_node * get_next()
      { return next; }

   /// return the previous XML node in the document
   const XML_node * get_prev() const
      { return prev; }

   /// return the previous XML node in the document
   XML_node * get_prev()
      { return prev; }

   /// return the type of this node
   Node_type get_node_type() const
        { return node_type; }

   /// return the (source-) string for this node
   UCS_string get_item() const
      { return UCS_string(src, src_pos, src_len); }

   /// return the start position of this node in src
   size_t get_src_pos() const
      { return src_pos; }

   /// return the number of characters in this node
   size_t get_src_len() const
      { return src_len; }

   /// return the tagname of this node
   UCS_string get_tagname() const;

   bool is_parsed() const
      { return (+APL_value); }

   /// unlink \b this node from the doubly-linked list that contains it
   XML_node * unlink()
      {
        prev->next = next;
        next->prev = prev;

        prev = this;
        next = this;

        return this;
      }

   /// append \b garbage to this garbage can (for later deletion)
   void append(XML_node * garbage)
      {
        // check that garbage was properly unlinked.
        Assert(garbage->next == garbage);
        Assert(garbage->prev == garbage);

        garbage->prev = this;
        garbage->next = next;
        next = garbage;
      }

   /// (debug-) print this node
   void print(std::ostream & out) const;

   /// parse an XML start or leaf tag, store result in \b this->APL_value
   /// On error set MORE_ERROR() and return \b true
   bool parse_tag();

   /// (debug-) print all nodes·
   static void print_all(std::ostream & out, const XML_node & anchor);

   /// return the type of this node as string
   const char * get_node_type_name() const;

   /// reduce nodes starting at \b start, return true on error
   static bool translate(XML_node & anchor, XML_node & garbage);

   /// collect nodes starting at \b anchor, return true on error
   static bool collect(XML_node & anchor, XML_node & garbage, Value * Z);

   /// add member \b name with value \b value to structured value Z
   static void add_member(Value * Z, const char * name, int number,
                          Value * value);

   /// merge the items from start(-tag) to (end(-tag) into start tag
   static bool merge_range(XML_node & start, XML_node & end,
                           XML_node & garbage_can);

   /// return true if uni is a validchar
   static bool is_name_char(Unicode uni);

   /// return true if uni is a valid char in an XML tag name
   static bool is_XML_char(Unicode uni);

   /// return true if uni is a valid first char in an XML tag name
   static bool is_first_name_char(Unicode uni);

   /// return the length of the tag at string_B + offset...
   static ShapeItem get_taglen(const UCS_string & string_B, ShapeItem offset);

   /// perform an in-place attribute value normalization (XML standard 3.3.3).
   /// On error set MORE_ERROR() and return \b true
   static bool normalize_attribute_value(UCS_string & attval);

protected:
   /// return true iff \b end_tag matches \b this start tag
   bool matches(const XML_node * end_tag) const;

   /// the source string of the entire xml document
   const UCS_string & src;   // B of ⎕XML B

   /// the start of this node in src
   const ShapeItem src_pos;

   /// the length of this node in src
   ShapeItem src_len;

   /// the next XML node in the document
   XML_node * next;

   /// the previous XML node in the document
   XML_node * prev;

   /// the type of this node
   Node_type node_type;

   /// the APL value for this node
   Value_P APL_value;

   /// the error location (if any) when constructing this node
   const char * err_loc;

   /// the level of this node in the document (top-level = 0)
   int level;

   /// the position of this node (at its level)
   int position;
};
//------------------------------------------------------------------------------
/**
   The system function ⎕XML
 */
/// The class implementing ⎕XML
class Quad_XML : public QuadFunction
{
public:
   /// Constructor.
   Quad_XML()
      : QuadFunction(TOK_Quad_XML)
   {}

   static Quad_XML * fun;          ///< Built-in function.
   static Quad_XML  _fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB()
   Token eval_AB(Value_P A, Value_P B) const;

   /// convert APL associative array to XML string
   static Value_P APL_to_XML(const Value & B);

   /// return the entities in B, sorted by their position prefix
   static void add_sorted_entities(std::vector<const UCS_string *> & entities,
                                   const Value & B);

   /// find the data cell (if any) with position prefix prefix idx in B.
   /// \b is_tag distinguishes (flat) attribute arrays from (recursive)
   /// subtag arrays
   static const Cell * find_entity(Unicode & type, ShapeItem pos,
                                   const Value & B);

   /// convert XML string to APL associative array
   static Value_P XML_to_APL(const Value & B);

   /// overloaded Function::eval_B()
   Token eval_B(Value_P B) const;

   /// return XML file (-name in B) converted to APL structured value
   Token convert_file(const Value & B) const;

   /// return a tree-view of the members in B
   static Value_P tree(const Value & B);

   /// return a sub-tree-view of the members in B at \b level
   static void tree(const Value & B, UCS_string & z, UCS_string & prefix);
};

#endif // __Quad_XML_DEFINED__

