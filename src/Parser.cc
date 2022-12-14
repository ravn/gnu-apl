/*
    This file is part of GNU APL, a free implementation of the
    ISO/IEC Standard 13751, "Programming Language APL, Extended"

    Copyright (C) 2008-2022  Dr. Jürgen Sauermann

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

#include <string.h>
#include <vector>

#include "CharCell.hh"
#include "ComplexCell.hh"
#include "Common.hh"
#include "FloatCell.hh"
#include "IntCell.hh"
#include "Output.hh"
#include "Parser.hh"
#include "PointerCell.hh"
#include "PrintOperator.hh"
#include "Symbol.hh"
#include "SystemLimits.hh"
#include "SystemVariable.hh"
#include "Value.hh"
#include "Tokenizer.hh"
#include "Workspace.hh"

//----------------------------------------------------------------------------
ErrorCode
Parser::parse(const UCS_string & input, Token_string & tos) const
{
   // convert input characters into token
   //
Token_string tos1;

   {
     Tokenizer tokenizer(pmode, LOC, macro);
     if (const ErrorCode ec = tokenizer.tokenize(input, tos1))   return ec;
   }

   // special case: single token (to speed up ⍎)
   //
   if (tos1.size() == 1)
      {
        const ErrorCode err = parse_statement(tos1);
        if (err == E_NO_ERROR)   tos.push_back(tos1[0]);
        return err;
      }

   return parse(tos1, tos);
}
//----------------------------------------------------------------------------
ErrorCode
Parser::parse(const Token_string & input, Token_string & tos) const
{
   Log(LOG_parse)
      {
        CERR << "parse 1 [" << input.size() << "]: ";
        input.print(CERR, true);
        CERR << endl;
      }

   // split input line into statements (separated by ◊)
   //
std::vector<Token_string *> statements;
   {
     Token_string * stat = new Token_string();
     loop(idx, input.size())
        {
          const Token & tok = input[idx];
          if (tok.get_tag() == TOK_DIAMOND)
             {
               statements.push_back(stat);
               stat = new Token_string();
             }
          else
             {
               stat->push_back(tok);
             }
        }
     statements.push_back(stat);
   }

   loop(s, statements.size())
      {
        Token_string * stat = statements[s];
        if (const ErrorCode err = parse_statement(*stat))
           {
             while (size_t(s) < statements.size())
               {
                 stat = statements[s++];
                 delete stat;
               }
             return err;
           }

        if (s)   tos.push_back(Token(TOK_DIAMOND));

        loop(t, stat->size())
           {
             tos.push_back(Token());
             tos[tos.size() - 1].move_1((*stat)[t], LOC);
           }
        delete stat;
      }

   return E_NO_ERROR;
}
//----------------------------------------------------------------------------
ErrorCode
Parser::parse_statement(Token_string & tos)
{
   // 1. convert (X) into X and ((X...)) into (X...)
   //
   remove_nongrouping_parantheses(tos);
   remove_void_token(tos);

   Log(LOG_parse)
      {
        CERR << "parse 2 [" << tos.size() << "]: ";
        tos.print(CERR, true);
      }

   // 2. convert groups like '(' val val...')' into single APL values
   // A group is defined as 2 or more values enclosed in '(' and ')'.
   // For example: (1 2 3) or (4 5 (6 7) 8 9)
   //
   for (int progress = true; progress;)
      {
        progress = collect_groups(tos);
        if (progress)   remove_void_token(tos);
      }

   Log(LOG_parse)
      {
        CERR << "parse 3 [" << tos.size() << "]: ";
        tos.print(CERR, true);
      }

   // 3. convert vectors like 1 2 3 or '1' 2 '3' into single APL values
   //
   collect_constants(tos);
   remove_void_token(tos);

   // special case: single APL value (to speed up ⍎)
   //
   if (tos.size() == 1)
      {
        Log(LOG_parse)   CERR << "parse 3a: single value " << tos[0] << endl;;
        return E_NO_ERROR;
      }

   Log(LOG_parse)
      {
        CERR << "parse 4 [" << tos.size() << "]: ";
        tos.print(CERR, true);
      }

   // 4. mark symbol left of ← as LSYMB
   //
   mark_lsymb(tos);
   Log(LOG_parse)
      {
        CERR << "parse 5 [" << tos.size() << "]: ";
        tos.print(CERR, true);
      }

   // 5. replace bitwise functons ⊤∧, ⊤∨, ⊤⍲, and ⊤⍱ by their bitwise variant
   //
   replace_bitwise_functions(tos);
   remove_void_token(tos);
   Log(LOG_parse)
      {
        CERR << "parse 6 [" << tos.size() << "]: ";
        tos.print(CERR, true);
      }

   // 6. update distances between (), [], and {}
   //
   {
     const ErrorCode ec = match_par_bra(tos, false);
     if (ec != E_NO_ERROR)
        {
          loop(t, tos.size())   tos[t].clear(LOC);
          return ec;
        }
   }

   return E_NO_ERROR;
}
//----------------------------------------------------------------------------
void
Parser::collect_constants(Token_string & tos)
{
   Log(LOG_collect_constants)
      {
        CERR << "collect_constants [" << tos.size() << " token] in: ";
        tos.print(CERR, true);
      }

   // convert several items in vector notation into a single APL value
   //
   loop (t, tos.size())
      {
        size_t to;
        switch(tos[t].get_tag())
           {
             case TOK_APL_VALUE1:
             case TOK_APL_VALUE3:
             case TOK_CHARACTER:
             case TOK_COMPLEX:
             case TOK_INTEGER:
             case TOK_REAL:
                  break;          // continue below

             default: continue;   // nex token
           }

        // at this point, t is the first item. Collect subsequenct items.
        //
        for (to = t + 1; to < tos.size(); ++to)
            {
#if 1
              // Note: ISO 13751 gives an example with 1 2 3[2] ↔ 2
              // 
              // In contrast, the binding rules stated in IBM's apl2lrm.pdf
              // would give 3[2] ↔ RANK ERROR.
              // 
              // We follow apl2lrm; to enable ISO behavior write #if 0 above.
              // 

              // if tos[to + 1] is [ then [ binds stronger than
              // vector notation and we stop collecting.
              //
              if (size_t(to + 1) < tos.size() &&
                  tos[to + 1].get_tag() == TOK_L_BRACK)   break;
#endif
              const TokenTag tag = tos[to].get_tag();

              if (tag == TOK_CHARACTER)    continue;
              if (tag == TOK_INTEGER)      continue;
              if (tag == TOK_REAL)         continue;
              if (tag == TOK_COMPLEX)      continue;
              if (tag == TOK_APL_VALUE1)   continue;
              if (tag == TOK_APL_VALUE3)   continue;

              // no more values: stop collecting
              //
              break;
            }

        create_value(tos, t, to - t);
      }

   Log(LOG_collect_constants)
      {
        CERR << "collect_constants [" << tos.size() << " token] out: ";
        tos.print(CERR, true);
      }
}
//----------------------------------------------------------------------------
bool
Parser::collect_groups(Token_string & tos)
{
   Log(LOG_collect_constants)
      {
        CERR << "collect_groups [" << tos.size() << " token] in: ";
        tos.print(CERR, true);
      }

int opening = -1;
   loop(t, tos.size())
       {
         switch(tos[t].get_tag())
            {
              case TOK_CHARACTER:
              case TOK_INTEGER:
              case TOK_REAL:
              case TOK_COMPLEX:
              case TOK_APL_VALUE1:
              case TOK_APL_VALUE3:
                   continue;

              case TOK_L_PARENT:
                   // we remember the last opening '(' so that we know where
                   // the group started when we see a ')'. This causes the
                   // deepest ( ... ) to be grouped first.
                   opening = t;
                   continue;

              case TOK_R_PARENT:
                   if (opening == -1)      continue;       // ')' without '('
                   if (opening == t - 1)   SYNTAX_ERROR;   // '(' ')'

                   tos[opening].clear(LOC);  // invalidate '('
                   tos[t].clear(LOC);        // invalidate ')'
                   create_value(tos, opening + 1, t - opening - 1);

                   // we removed parantheses, so we close the value.
                   //
                   if (tos[opening + 1].get_tag() == TOK_APL_VALUE3)
                      tos[opening + 1].ChangeTag(TOK_APL_VALUE1);

                   Log(LOG_collect_constants)
                      {
                        CERR << "collect_groups [" << tos.size()
                             << " token] out: ";
                        tos.print(CERR, true);
                      }
                   return true;

              default: // nothing group'able
                   opening = -1;
                   continue;
            }
       }

   return false;
}
//----------------------------------------------------------------------------
int
Parser::find_closing_bracket(const Token_string & tos, int pos)
{
   Assert(tos[pos].get_tag() == TOK_L_BRACK);

int others = 0;

   for (size_t p = pos + 1; p < tos.size(); ++p)
       {
         Log(LOG_find_closing)
            CERR << "find_closing_bracket() sees " << tos[p] << endl;

         if (tos[p].get_tag() == TOK_R_BRACK)
            {
             if (others == 0)   return p;
             --others;
            }
         else if (tos[p].get_tag() == TOK_R_BRACK)   ++others;
       }

   SYNTAX_ERROR;
}
//----------------------------------------------------------------------------
int
Parser::find_opening_bracket(const Token_string & tos, int pos)
{
   Assert(tos[pos].get_tag() == TOK_R_BRACK);

int others = 0;

   for (int p = pos - 1; p >= 0; --p)
       {
         Log(LOG_find_closing)
            CERR << "find_opening_bracket() sees " << tos[p] << endl;

         if (tos[p].get_tag() == TOK_L_BRACK)
            {
             if (others == 0)   return p;
             --others;
            }
         else if (tos[p].get_tag() == TOK_R_BRACK)   ++others;
       }

   SYNTAX_ERROR;
}
//----------------------------------------------------------------------------
int
Parser::find_closing_parent(const Token_string & tos, int pos)
{
   Assert1(tos[pos].get_Class() == TC_L_PARENT);

int others = 0;

   for (size_t p = pos + 1; p < tos.size(); ++p)
       {
         Log(LOG_find_closing)
            CERR << "find_closing_bracket() sees " << tos[p] << endl;

         if (tos[p].get_Class() == TC_R_PARENT)
            {
             if (others == 0)   return p;
             --others;
            }
         else if (tos[p].get_Class() == TC_R_PARENT)   ++others;
       }

   SYNTAX_ERROR;
}
//----------------------------------------------------------------------------
int
Parser::find_opening_parent(const Token_string & tos, int pos)
{
   Assert(tos[pos].get_Class() == TC_R_PARENT);

int others = 0;

   for (int p = pos - 1; p >= 0; --p)
       {
         Log(LOG_find_closing)
            CERR << "find_opening_bracket() sees " << tos[p] << endl;

         if (tos[p].get_Class() == TC_L_PARENT)
            {
             if (others == 0)   return p;
             --others;
            }
         else if (tos[p].get_Class() == TC_R_PARENT)   ++others;
       }

   SYNTAX_ERROR;
}
//----------------------------------------------------------------------------
void
Parser::remove_nongrouping_parantheses(Token_string & tos)
{
   //
   // 1. replace ((X...)) by: (X...)
   // 2. replace (X)      by: X for a single token X,
   //
   for (bool progress = true; progress; progress = false)
       {
         loop(t, int(tos.size()) - 2)
             {
               if (tos[t].get_Class() != TC_L_PARENT)   continue;

               const int closing = find_closing_parent(tos, t);

               // check for case 1.
               //
               if (tos[t + 1].get_Class() == TC_L_PARENT)
                  {
                    // tos[t] is (( ...
                    //
                    const int closing_1 = find_closing_parent(tos, t + 1);
                    if (closing == (closing_1 + 1))
                       {
                         // tos[closing_1] is )) ...
                         // We have case 1. but not for example ((...)(...))
                         // remove redundant tos[t] and tos[closing] because
                         // ((...)) are not "not separating"
                         //
                         progress = true;
                         tos[t].clear(LOC);
                         tos[closing].clear(LOC);
                         continue;
                       }
                  }

               // check for case 2.
               //
               if (closing != (t + 2))   continue;


               // case 2. We have tos[t] = ( X ) ...
               //
               // (X) : "not grouping" if X is a scalar. 
               // If X is non-scalar, enclose it
               //
               progress = true;
               tos[t + 2].move_1(tos[t + 1], LOC);

               // we "remember" the nongrouping parantheses to disambiguate
               // e,g, SYM/xxx from (SYM)/xxx
               //
               if (tos[t + 2].get_tag() == TOK_SYMBOL)
                  tos[t + 2].ChangeTag(TOK_P_SYMB);
               tos[t + 1].clear(LOC);
               tos[t].clear(LOC);
               ++t;   // skip tos[t + 1]
             }
       }
}
//----------------------------------------------------------------------------
void
Parser::replace_bitwise_functions(Token_string & tos)
{
   // replace bitwise functions (⊤∧ ⊤∨ ⊤⍲ ⊤⍱ ⊤= or ⊤≠) by their
   // own token
   //
   loop(t, int(tos.size()) - 1)
       {
         if (tos[t].get_tag() != TOK_F12_ENCODE)   continue;

         switch(tos[t+1].get_tag())
            {
              case TOK_F2_AND:
                   tos[t] = Token(TOK_F2_AND_B, &Bif_F2_AND_B::_fun);
                   tos[++t].clear(LOC);
                   continue;

              case TOK_F2_OR:
                   tos[t] = Token(TOK_F2_OR_B, &Bif_F2_OR_B::_fun);
                   tos[++t].clear(LOC);
                   continue;

              case TOK_F2_NAND:
                   tos[t] = Token(TOK_F2_NAND_B,&Bif_F2_NAND_B::_fun);
                   tos[++t].clear(LOC);
                   continue;

              case TOK_F2_NOR:
                   tos[t] = Token(TOK_F2_NOR_B, &Bif_F2_NOR_B::_fun);
                   tos[++t].clear(LOC);
                   continue;

              case TOK_F2_EQUAL:
                   tos[t] = Token(TOK_F2_EQUAL_B, &Bif_F2_EQUAL_B::_fun);
                   tos[++t].clear(LOC);
                   continue;

              case TOK_F2_UNEQU:
                   tos[t] = Token(TOK_F2_UNEQ_B, &Bif_F2_UNEQ_B::_fun);
                   tos[++t].clear(LOC);
                   continue;

              default: break;
            }
       }
}
//----------------------------------------------------------------------------
bool
Parser::check_if_value(const Token_string & tos, int pos)
{
   // figure if tos[pos] (the token left of /. ⌿. \. or ⍀) is the end of a
   // function (and then return false) or the end of a value (and then
   // return true).
   //
   switch(tos[pos].get_Class())
      {
        case TC_ASSIGN:     // e.g. ←/       (actually syntax error)
        case TC_R_ARROW:    // e.g. →/       (actually syntax error)
        case TC_L_BRACK:    // e.g. [/       (actually syntax error)
        case TC_END:        // e.g.  /       (actually syntax error)
        case TC_L_PARENT:   // e.g. (/       (actually syntax error)
        case TC_VALUE:      // e.g. 5/
        case TC_RETURN:     // e.g.  /       (actually syntax error)
        case TC_OPER2:      // e.g. ./
             return true;   // tos[pos] is at the end of a value

        case TC_R_BRACK:    // e.g. +[1]/2 or 2/[1]2
             {
               const int pos1 = find_opening_bracket(tos, pos);
               if (pos1 == 0)   return true;   // this is a syntax error
               return check_if_value(tos, pos1 - 1);
             }

        case TC_R_PARENT:   // e.g. (2+3)/2 or 1 2 3 (2+3)/2
             {
               if (pos == 0)   return true;   // (actually syntax error)
               return check_if_value(tos, pos - 1);
             }

        case TC_SYMBOL:     // e.g. A/2 or FOO/2
             if (tos[pos].get_tag() == TOK_ALPHA)     return true;
             if (tos[pos].get_tag() == TOK_CHI)       return true;
             if (tos[pos].get_tag() == TOK_OMEGA)     return true;
             if (tos[pos].get_tag() == TOK_ALPHA_U)   return false;
             if (tos[pos].get_tag() == TOK_OMEGA_U)   return false;
             return (tos[pos].get_tag() == TOK_P_SYMB);   // if value

        default: break;
      }

   return false;   // tos[pos] is at the end of a function
}
//----------------------------------------------------------------------------
void
Parser::remove_void_token(Token_string & tos)
{
size_t dst = 0;

   loop(src, tos.size())
       {
         if (tos[src].get_tag() == TOK_VOID)   continue;
         if (src != ShapeItem(dst))   tos[dst].move_1(tos[src], LOC);
         ++dst;
       }

   tos.resize(dst);
}
//----------------------------------------------------------------------------
ErrorCode
Parser::match_par_bra(Token_string & tos, bool backwards)
{
std::vector<ShapeItem> stack;
   loop(s, tos.size())
       {
         const ShapeItem t = backwards ? (tos.size() - 1) - s : s;
         ErrorCode ec;
         TokenClass tc_peer;
         switch(tos[t].get_Class())
           {
             // for [ ( or { push the position onto stack
             case TC_L_BRACK:  if (tos[t].get_tag() != TOK_L_BRACK)   continue;
             case TC_L_PARENT:
             case TC_L_CURLY:  stack.push_back(t);
                               continue;

             case TC_R_BRACK:  ec = E_UNBALANCED_BRACKET;
                               tc_peer = TC_L_BRACK;
                               break;

             case TC_R_PARENT: ec = E_UNBALANCED_PARENT;
                               tc_peer = TC_L_PARENT;
                               break;

             case TC_R_CURLY:  ec = E_UNBALANCED_CURLY;
                               tc_peer = TC_L_CURLY;
                               break;

             default:          continue;
           }

          // at this point, a closing ), ], or } was detected
          //
          if (stack.size() == 0)   return ec;

           const ShapeItem t1 = stack.back();
           stack.pop_back();

          if (tos[t1].get_Class() != tc_peer)   return ec;

          const ShapeItem diff = (t > t1) ? t - t1 : t1 - t;
          tos[t].set_int_val2(diff);
          tos[t1].set_int_val2(diff);
       }

   // if there are unmatched items: return syntax error of the outer token
   //
   if (stack.size())
      {
        const TokenClass outer = tos[stack[0]].get_Class();
        if (outer == TC_L_BRACK)    return E_UNBALANCED_BRACKET;
        if (outer == TC_L_PARENT)   return E_UNBALANCED_PARENT;
        return E_UNBALANCED_CURLY;
      }

   return E_NO_ERROR;
}
//----------------------------------------------------------------------------
void
Parser::create_value(Token_string & tos, int pos, int count)
{
   Log(LOG_create_value)
      {
        CERR << "create_value(" << __LINE__ << ") tos[" << tos.size()
             <<  "]  pos " << pos << " count " << count << " in:";
        tos.print(CERR, true);
      }

   if (count == 1)   create_scalar_value(tos[pos]);
   else              create_vector_value(tos, pos, count);

   Log(LOG_create_value)
      {
        CERR << "create_value(" << __LINE__ << ") tos[" << tos.size()
             <<  "]  pos " << pos << " count " << count << " out:";
        tos.print(CERR, true);
      }
}
//----------------------------------------------------------------------------
void
Parser::create_scalar_value(Token & output)
{
   switch(output.get_tag())
      {
        case TOK_CHARACTER:
             {
               Value_P scalar(LOC);

               scalar->next_ravel_Char(output.get_char_val());
               scalar->check_value(LOC);
               Token tok(TOK_APL_VALUE3, scalar);
               output.move_1(tok, LOC);
             }
             return;

        case TOK_INTEGER:
             {
               Token tok(TOK_APL_VALUE3, IntScalar(output.get_int_val(), LOC));
               output.move_1(tok, LOC);
             }
             return;

        case TOK_REAL:
             {
               Token tok(TOK_APL_VALUE3,
                         FloatScalar(output.get_flt_val(), LOC));
               output.move_1(tok, LOC);
             }
             return;

        case TOK_COMPLEX:
             {
               Token tok(TOK_APL_VALUE3,
                         ComplexScalar(output.get_cpx_real(),
                                       output.get_cpx_imag(), LOC));
               output.move_1(tok, LOC);
             }
             return;

        case TOK_APL_VALUE1:
        case TOK_APL_VALUE3:
             return;

        default: break;
      }

   CERR << "Unexpected token " << output.get_tag() << ": " << output << endl;
   Assert(0 && "Unexpected token");
}
//----------------------------------------------------------------------------
void
Parser::create_vector_value(Token_string & tos, int pos, int count)
{
Value_P Z(count, LOC);

   loop(l, count)
       {
         Token & tok = tos[pos + l];

         switch(tok.get_tag())
            {
              case TOK_CHARACTER:
                   Z->next_ravel_Char(tok.get_char_val());
                   tok.clear(LOC);   // invalidate token
                   break;

              case TOK_INTEGER:
                   Z->next_ravel_Int(tok.get_int_val());
                   tok.clear(LOC);   // invalidate token
                   break;

              case TOK_REAL:
                   Z->next_ravel_Float(tok.get_flt_val());
                   tok.clear(LOC);   // invalidate token
                   break;

              case TOK_COMPLEX:
                   Z->next_ravel_Complex(tok.get_cpx_real(),
                                         tok.get_cpx_imag());
                   tok.clear(LOC);   // invalidate token
                   break;

            case TOK_APL_VALUE1:
            case TOK_APL_VALUE3:
                 Z->next_ravel_Value(tok.get_apl_val().get());
                 tok.clear(LOC);   // invalidate token
                 break;

              default: FIXME;
            }
       }

   Z->check_value(LOC);
Token tok(TOK_APL_VALUE3, Z);

   tos[pos].move_1(tok, LOC);

   Log(LOG_create_value)
      {
        CERR << "create_value [" << tos.size() << " token] out: ";
        tos.print(CERR, true);
      }
}
//----------------------------------------------------------------------------
/// in tos, (re-) mark the symbols left of ← as left symbols
void
Parser::mark_lsymb(Token_string & tos)
{
   loop(ass, tos.size())
      {
        if (tos[ass].get_Class() != TC_ASSIGN)   continue;

        // found ← in VAR VAR)← move backwards. Before that we handle the special
        // case of vector specification, i.e. (SYM SYM ... SYM) ← value
        //
        if (ass >= 3 && tos[ass - 1].get_Class() == TC_R_PARENT &&
                        tos[ass - 2].get_Class() == TC_SYMBOL   &&
                        tos[ass - 3].get_Class() == TC_SYMBOL)
           {
             // first make sure that this is really a vector specification.
             //
             const int syms_to = ass - 2;
             int syms_from = ass - 3;
             bool is_vector_spec = true;
             bool is_selective_spec = false;
             for (int a1 = ass - 4; a1 >= 0; --a1)
                 {
                   const TokenClass tc = tos[a1].get_Class();
                   if (tc == TC_L_PARENT)   // end of (...)←
                      {
                        syms_from = a1 + 1;
                        break;
                      }
                   if (tc == TC_SYMBOL)     continue; 
                   if (tc == TC_FUN12 || tc == TC_OPER1 || tc == TC_OPER2)
                      {
                        is_selective_spec = true;
                        is_vector_spec = false;
                        break;
                      }

                   // something else
                   //
                   is_vector_spec = false;
                   break;
                 }

             if (is_selective_spec == is_vector_spec)   // none or both
                {
                  MORE_ERROR() <<
                      "Left of )← seems to be neither a vector "
                      "specification nor a selective specvification";
                  LEFT_SYNTAX_ERROR;
                }

             // if this is a vector specification, then mark all symbols
             // inside ( ... ) as TOK_LSYMB2.
             //
             if (is_vector_spec)
                {
                  for (int a1 = syms_from; a1 <= syms_to; ++a1)
                      tos[a1].ChangeTag(TOK_LSYMB2);
                }

             continue;
           }

        int bracks = 0;
        for (int prev = ass - 1; prev >= 0; --prev)
            {
              switch(tos[prev].get_Class())
                 {
                   case TC_R_BRACK: ++bracks;     break;
                   case TC_L_BRACK: --bracks;     break;
                   case TC_SYMBOL:  if (bracks)   break;   // inside [ ... ]
                                    if (tos[prev].get_tag() == TOK_SYMBOL)
                                       {
                                         tos[prev].ChangeTag(TOK_LSYMB);
                                       }
                                    // it could be that a system variable
                                    // is assigned, e.g. ⎕SVE←0. In that case
                                    // (actually always) we simply stop here.
                                    //

                                    /* fall-through */

                   case TC_END:     prev = 0;   // stop prev loop
                                    break;

                   default: break;
                 }
            }
      }
}
//----------------------------------------------------------------------------
