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

#include <vector>

#include "Quad_XML.hh"
#include "Value.hh"
#include "Workspace.hh"

Quad_XML  Quad_XML::_fun;
Quad_XML * Quad_XML::fun = &Quad_XML::_fun;

//-----------------------------------------------------------------------------
/// a helper class (doubly linked list) for parsing XML
class XML_node
{
public:
   /// constructor
   XML_node(XML_node * anchor, const UCS_string & src,
            ShapeItem spos, ShapeItem slen);

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

   /// return the number of characters in this node
   size_t get_src_len() const
      { return src_len; }

   /// return the tagname of this node
   UCS_string get_tagname() const;

   bool is_parsed() const
      { return (+value); }

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
   void print(ostream & out) const;

   /// (debug-) print all nodes 
   static void print_all(ostream & out, const XML_node & anchor);

   /// return the type of this node as string
   const char * get_node_type_name() const;

   /// reduce nodes starting at \b start, return true on error
   static bool translate(XML_node & anchor, XML_node & garbage);

   /// collect nodes starting at \b anchor, return true on error
   static bool collect(XML_node & anchor, XML_node & garbage, Value * Z);

   /// add ∆-something member to Z
   static void add_member(Value * Z, const char * delta_x, size_t num,
                          Value * value);

   /// merge the items drom start(-tag) to (end(-tag) into start tag
   static bool merge_range(XML_node & start, XML_node & end, XML_node & garbage);

   /// return true if uni is a valid char in an XML tag name
   static bool is_tagname_char(Unicode uni);

   /// return true if uni is a valid first char in an XML tag name
   static bool is_first_tagname_char(Unicode uni);

   /// parse an XML start or leaf tag, store result in \b this->value
   bool parse_tag();

protected:
   /// the source string of the entire xml document
   const UCS_string & src;   // B of ⎕XML B

   /// the start of this node in src
   const ShapeItem src_pos;

   /// the length of this node in src
   const ShapeItem src_len;

   /// the next XML node in the document
   XML_node * next;

   /// the previous XML node in the document
   XML_node * prev;

   /// the type of this node
   Node_type node_type;

   /// the APL value for this node
   Value_P value;

   /// the error location (if any) when constructing this node
   const char * err_loc;


   /// the level of this node in the document (top-level = 0)
   int level;

   /// the position of this node (at its level)
   int position;
};
//-----------------------------------------------------------------------------
XML_node::XML_node(XML_node * anchor, const UCS_string & string_B,
                   ShapeItem spos, ShapeItem slen)
: src(string_B),
  src_pos(spos),
  src_len(slen),
  next(this),
  prev(this),
  err_loc(0),
  level(-1),
  position(-1)
{
  if (anchor)   // unless this node is the root node
     {
       prev = anchor->prev;
       anchor->prev = this;

       next = anchor;
       prev->next = this;
     }

   // figure the node type
   //
const UCS_string item(src, spos, slen);
   if (item[0] != '<')
      {
        node_type = NT_text;
        return;
      }

   node_type = NT_error;
   if (slen < 3)
      {
        err_loc = LOC;
        MORE_ERROR() << "⎕XML: Tag too short: " << UCS_string(src, spos, slen);
        return;
      }

   // <...
   //
   if (!item.ends_with(">"))
      {
        err_loc = LOC;
        MORE_ERROR() << "⎕XML: No tag end: " << item;
        return;
      }

   // <...>
   //
const Unicode ucs1 = item[1];   // the character after <
   if (ucs1 == '!')   // XML declaration or comment <! ... >
      {
        if (slen < 7)   // at least <!---->
           {
             err_loc = LOC;
             MORE_ERROR() << "⎕XML: comment not terminated: " << item;
             return;
           }

        if (item.starts_with("<!DOCTYPE") && item.ends_with(">"))
           {
             node_type = NT_doctype;
             return;
           }

        if (!item.starts_with("<!--"))   // XML comment
           {
             err_loc = LOC;
             MORE_ERROR() <<
             "⎕XML: bad comment start (expecting <!--) : " << item;
             return;
           }

        else if (!item.ends_with("-->"))
           {
             err_loc = LOC;
             MORE_ERROR() <<
             "⎕XML: bad comment end (expecting -->): " << item;
             return;
           }
        node_type = NT_comment;
      }
   else if (ucs1 == '?')   // <? ... >
      {
        if (slen < 7)   // at least <?xml?>
           {
             err_loc = LOC;
             MORE_ERROR() <<
             "⎕XML: processing instruction not terminated: " << item;
             return;
           }

        else if (!item.starts_with("<?"))
           {
             err_loc = LOC;
             MORE_ERROR() <<
             "⎕XML: bad declaration start (expecting <?xml): " << item;
             return;
           }

        else if (!item.ends_with("?>"))
           {
             err_loc = LOC;
             MORE_ERROR() <<
             "⎕XML: bad declaration end (expecting ?>): " << item;
             return;
           }
        node_type = NT_declaration;
      }
   else if (ucs1 == '/')   // </ ... >
      {
        const Unicode ucs2 = item[2];
        if (ucs2 < 'A' || (ucs2 > 'Z' && ucs2 < 'a') || ucs2 > 'z')
           {
             err_loc = LOC;
             MORE_ERROR() <<
             "⎕XML: bad tag name in closing tag : " << item;
             return;
           }
        node_type = NT_end_tag;
      }
   else if (item[item.size() - 2] == '/')   // leaf tag
      {
        if (ucs1 < 'A' || (ucs1 > 'Z' && ucs1 < 'a') || ucs1 > 'z')
           {
             err_loc = LOC;
             MORE_ERROR() <<
             "⎕XML: bad tag name in leaf tag: " << item;
             return;
           }
        node_type = NT_leaf_tag;
      }
   else                              // start tag
      {
        if (ucs1 < 'A' || (ucs1 > 'Z' && ucs1 < 'a') || ucs1 > 'z')
           {
             err_loc = LOC;
             MORE_ERROR() <<
             "⎕XML: bad tag name in start tag: " << item;
             return;
           }
        node_type = NT_start_tag;
      }

   Assert(node_type != NT_error);
}
//-----------------------------------------------------------------------------
void
XML_node::print(ostream & out) const
{
   if (err_loc)   CERR << "    errloc: " << err_loc << endl;

const UCS_string item(src, src_pos, src_len);
   out << get_node_type_name() << "[" << level << "] '" << item << "'" << endl;
}
//-----------------------------------------------------------------------------
void
XML_node::print_all(ostream & out, const XML_node & anchor)
{
   for (const XML_node * x = anchor.get_next(); x != &anchor; x = x->get_next())
       {
          x->print(CERR);
       }
}
//-----------------------------------------------------------------------------
const char *
XML_node::get_node_type_name() const
{
   switch(node_type)
      {
        case NT_error:       return "NT_err ";
        case NT_text:        return "NT_text";
        case NT_leaf_tag:    return "NT_leaf";
        case NT_start_tag:   return "NT_stag";
        case NT_end_tag:     return "NT_etag";
        case NT_comment:     return "NT_comm";
        case NT_declaration: return "NT_decl";
        case NT_doctype:     return "NT_doct";
      }

   return "--???";
}
//=============================================================================
Token
Quad_XML::eval_B(Value_P B) const
{
   if (B->get_rank() != 1)   RANK_ERROR;

const UCS_string string_B(*B);

const ShapeItem len_B = B->element_count();
bool inside_tag = false;   // inside < ... >
bool inside_dq  = false;   // inside "..."
bool inside_sq  = false;   // inside '...'

XML_node anchor(0,  string_B, 0, 0);
XML_node garbage(0, string_B, 0, 0);
ShapeItem start = 0;

   // create a (flat) doubly-linked list, starting at anchor, and containing
   // all XML_nodes of B.
   //
   Workspace::more_error().clear();
   loop(b, len_B)
       {
         const Unicode uni = B->get_ravel(b).get_char_value();
         if (uni == '<')
            {
               if (inside_tag)
                  {
                    if (inside_sq || inside_dq)
                       MORE_ERROR() <<
                           "⎕XML B: Unescaped '<' in attribute value";
                    else
                       MORE_ERROR() <<
                           "⎕XML B: Unescaped '<' in attribute name";
                    DOMAIN_ERROR;
                  }
               inside_tag = true;
               new XML_node(&anchor, string_B, start, b - start);
               start = b;
            }
           else if (inside_tag)
            {
              if (uni == '>')
                 {
                   inside_tag = false;
                   new XML_node(&anchor, string_B, start, b + 1 - start);
                   start = b + 1;
                 }
              else if (uni == '"' && !inside_sq)
                 {
                   inside_dq = !inside_dq;
                 }
              else if (uni == '\'' && !inside_dq)
                 {
                   inside_sq = !inside_sq;
                 }
            }

         // else: unstructured text. Do nothing.
       }

   if (start != len_B)   // trailing unstructured text
      new XML_node(&anchor, string_B, start, len_B - start);

   if (XML_node::translate(anchor, garbage))   DOMAIN_ERROR;

Value_P Z = EmptyStruct(LOC);
   if (XML_node::collect(anchor, garbage, Z.get()))  DOMAIN_ERROR;

   while (anchor.get_next() != &anchor)     delete anchor.get_next()->unlink();
   while (garbage.get_next() != &garbage)   delete garbage.get_next()->unlink();

   return Token(TOK_APL_VALUE1, Z);
}
//-----------------------------------------------------------------------------
bool
XML_node::translate(XML_node & anchor, XML_node & garbage)
{
int level = 0;
ShapeItem count = 0;

   // reduce text and leaf nodes...
   //
   for (XML_node * node = anchor.get_next(); node != &anchor;
        node = node->get_next())
       {
         node->level = level;
         switch(node->get_node_type())
            {
              case NT_text:
              case NT_comment:
              case NT_declaration:
              case NT_doctype:
                   if (node->get_src_len())   // mot empty
                      {
                        const UCS_string item = node->get_item();
                        node->value = Value_P(item, LOC);
                      }
                   else   // empty string
                      {
                        node = node->get_prev();   // move back
                        garbage.append(node->get_next()->unlink());
                      }
                   break;

              case NT_start_tag:
                   ++level;
                   /* fall through */
              case NT_leaf_tag:
                   if (node->parse_tag())   DOMAIN_ERROR;
                   break;


              case NT_end_tag:
                   node->level = --level;
                   break;

              case NT_error: FIXME;
            }

         ++count;
       }

   return false;   // OK
}
//-----------------------------------------------------------------------------
bool
XML_node::collect(XML_node & anchor, XML_node & garbage, Value * Z)
{
XML_node * doctype = 0;
XML_node * root = 0;

vector<XML_node *> stack;   // a stack of start tags
vector<int> pos_stack;   // a stack of start tags

int position = 0;
   for (XML_node * node = anchor.get_next(); node != &anchor;
        node = node->get_next())
       {
         node->position = position++;
         switch(node->get_node_type())
            {
              case NT_declaration:
                   {
                     if (stack.size())   // only allowed at top-level
                        {
                          MORE_ERROR() <<
                          "⎕XML: XML declaration below top-level: "
                          << node->get_item();
                          DOMAIN_ERROR;
                        }

                     add_member(Z, "∆declaration", position, node->value.get());
                     node = node->get_prev();   // move back
                     garbage.append(node->get_next()->unlink());
                   }
                   break;

              case NT_doctype:
                   {
                     if (stack.size())   // only allowed at top-level
                        {
                          MORE_ERROR() <<
                          "⎕XML: !DOCTYPE below top-level: " << node->get_item();
                          DOMAIN_ERROR;
                        }

                     if (doctype)
                        {
                          MORE_ERROR() << "⎕XML: multiple !DOCTYPE declarations";
                          DOMAIN_ERROR;
                        }

                     doctype = node;
                     node = node->get_prev();
                     garbage.append(node->get_next()->unlink());
                   }
                   break;

              case NT_comment:
                   {
                     if (stack.size() == 0)   // only at top-level
                        {
                          add_member(Z, "∆comment", position, node->value.get());
                          node = node->get_prev();
                          garbage.append(node->get_next()->unlink());
                        }
                   }
                   break;

              case NT_text:
                   {
                     if (stack.size() == 0)   // only at top-level
                        {
                          add_member(Z, "∆text", position, node->value.get());
                          node = node->get_prev();
                          garbage.append(node->get_next()->unlink());
                        }
                   }
                   break;

              case NT_start_tag:
                   {
                     if (root == 0)   root = node;
                     Assert(stack.size() == size_t(node->level));  // translate()
                     stack.push_back(node);
                     pos_stack.push_back(position);
                     position = 0;
                   }
                   break;

              case NT_end_tag:
                   {
                     XML_node * start = stack.back();
                     stack.pop_back();
                     position = pos_stack.back();
                     pos_stack.pop_back();
                     merge_range(*start, *node, garbage);
                     node = start;
                   }
                   break;

              case NT_leaf_tag:
                   break;

              case NT_error: FIXME;
            }
       }

   if (doctype)   add_member(Z, "∆doctype", position, doctype->value.get());

const char * root_name = "∆root";
   if (root)
      {
        UTF8_string root_utf(root->get_tagname());
        root_name = root_utf.c_str();
      }

   add_member(Z, root_name, 0, anchor.get_next()->value.get());

   return false;   // OK
}
//-----------------------------------------------------------------------------
void
XML_node::add_member(Value * Z, const char * delta_x, size_t num, Value * value)
{
UTF8_string delta_utf(delta_x);
UCS_string delta_ucs(delta_utf);
   if (num)
      {
        delta_ucs += UNI_OVERBAR;
        delta_ucs.append_number(num);
      }
   Z->add_member(delta_ucs, value);
}
//-----------------------------------------------------------------------------
bool
XML_node::merge_range(XML_node & start, XML_node & end, XML_node & garbage)
{
   Assert(+start.value);   // ∆tagname
   Assert(start.value->is_structured());

   for (;;)
       {
         XML_node & sub = *(start.get_next());

       garbage.append(sub.unlink());

       if (sub.get_node_type() == NT_end_tag)   break;

       Assert(+sub.value);
       switch(sub.get_node_type())
          {
            case NT_text:
                 add_member(start.value.get(), "∆text", sub.position,
                              sub.value.get());
                 break;

            case NT_comment:
                 add_member(start.value.get(), "∆comment", sub.position,
                              sub.value.get());
                 break;

            case NT_leaf_tag:
                 start.value->add_member(sub.get_tagname(), sub.value.get());
                 break;


            // these should not occur, unless the algorithm is faulty.
            //
            case NT_start_tag:
            case NT_declaration:
            case NT_doctype:
            case NT_error:
            case NT_end_tag: FIXME;
          }
       }

   return false;   // OK
}
//-----------------------------------------------------------------------------
bool
XML_node::parse_tag()
{
   // return Z which is a sytructured variable with member
   // ∆tagname set to the name in the tag and
   // attname-N to the names of the attributes
   //
value = EmptyStruct(LOC);

size_t name_len = 0;
   while (is_tagname_char(src[src_pos + 1 + name_len]))   ++name_len;

const UCS_string delta_tagname(UTF8_string("∆tagname"));
const UCS_string Z_name("Z");
vector<const UCS_string *> members;
   members.push_back(&delta_tagname);
   members.push_back(&Z_name);   // not used but needed

Value * member_owner = 0;
Cell * cell = value->get_member(members, member_owner, true, false);
   Assert(member_owner);
   Assert(member_owner == value.get());
const UCS_string tag_name(src, src_pos + 1, name_len);
Value_P Z1(tag_name, LOC);
   new (cell)   PointerCell(Z1.get(), value.getref());

   return false;   // OK
}
//-----------------------------------------------------------------------------
UCS_string
XML_node::get_tagname() const
{
size_t start = 0;
   if (node_type == NT_start_tag || node_type == NT_leaf_tag)   // <TAGNAME ...
      {
        start = src_pos + 1;
      }
   else if (node_type == NT_end_tag)                       // </TAGNAME
      {
        start = src_pos + 2;
      }
   else
      {
        Q1(get_node_type_name())
        FIXME;
      }

size_t end = start;
   if (is_first_tagname_char(src[start]))   ++end;
   while (is_tagname_char(src[end]))        ++end;

   if (start == end)   // no valid tag name
      {
        MORE_ERROR() << "⎕XML: Bad tag name in: " << get_item();
        DOMAIN_ERROR;
      }

UCS_string ret(src, start, end - start);
   ret += UNI_OVERBAR;
   ret.append_number(position);
   return ret;
}
//-----------------------------------------------------------------------------
bool
XML_node::is_tagname_char(Unicode uni)
{
   // XML standard chapter 2.3 "Common Syntactic Constructs"
   //
   if (is_first_tagname_char(uni))       return true;
   if (strchr("-.0123456789", uni))      return true;
   if (uni == 0xBF)                      return true;
   if (uni >= 0x0300 && uni <= 0x036F)   return true;
   if (uni >= 0x203F && uni <= 0x2040)   return true;
   return false;
}
//-----------------------------------------------------------------------------
const int first_ranges[] =
{
     0xC0,    0xD6,
     0xD8,    0xF6,
     0xF8,   0x2FF,
    0x370,   0x37D,
    0x37F,  0x1FFF,
   0x200C,  0x200D,
   0x2070,  0x218F,
   0x2C00,  0x2FEF,
   0x3001,  0xD7FF,
   0xF900,  0xFDCF,
   0xFDF0,  0xFFFD,
  0x10000, 0xEFFFF
};

bool
XML_node::is_first_tagname_char(Unicode uni)
{
   // XML standard chapter 2.3 "Common Syntactic Constructs"
   //
   if (strchr(":ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "_abcdefghijklmnopqrstuvwxyz", uni))   return true;

   for (size_t j = 0; j < sizeof(first_ranges); )
       {
         if (uni <  first_ranges[j++])   return false;   // below range start
         if (uni <= first_ranges[j++])   return true;    // below range end
       }

   return false;
}
//=============================================================================
Token
Quad_XML::eval_AB(Value_P A, Value_P B) const
{
   TODO;
   return Token();
}
//=============================================================================
