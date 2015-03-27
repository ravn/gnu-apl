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

#include "Bif_OPER2_RANK.hh"
#include "Common.hh"
#include "DerivedFunction.hh"
#include "Executable.hh"
#include "IndexExpr.hh"
#include "Prefix.hh"
#include "StateIndicator.hh"
#include "Symbol.hh"
#include "UserFunction.hh"
#include "ValueHistory.hh"
#include "Workspace.hh"

//-----------------------------------------------------------------------------
Prefix::Prefix(StateIndicator & _si, const Token_string & _body)
   : si(_si),
     put(0),
     saved_lookahead(Token(TOK_VOID), Function_PC_invalid),
     body(_body),
     PC(Function_PC_0),
     assign_state(ASS_none),
     lookahead_high(Function_PC_invalid)
{
}
//-----------------------------------------------------------------------------
void
Prefix::clean_up()
{
   loop(s, size())
      {
        Token tok = at(s).tok;
        if (tok.get_Class() == TC_VALUE)
           {
             tok.extract_apl_val(LOC);
           }
        else if (tok.get_ValueType() == TV_INDEX)
           {
             tok.get_index_val().extract_all();
           }
      }

   put = 0;
}
//-----------------------------------------------------------------------------
void
Prefix::syntax_error(const char * loc)
{
   // move the PC back to the beginning of the failed statement
   //
   while (PC > 0)
      {
        --PC;
        if (body[PC].get_Class() == TC_END)
           {
             ++PC;
             break;
           }
      }

   // clear values in FIFO
   //
   loop (s, size())
      {
        Token & tok = at(s).tok;
        if (tok.get_Class() == TC_VALUE)
           {
             Value_P val = tok.get_apl_val();
          }
      }

   // see if error was caused by a function not returning a value.
   // in that case we throw a value error instead of a syntax error.
   //
   loop (s, size())
      {
        if (at(s).tok.get_Class() == TC_VOID)
           throw_apl_error(E_VALUE_ERROR, loc);
      }

   throw_apl_error(get_assign_state() == ASS_none ? E_SYNTAX_ERROR
                                                  : E_LEFT_SYNTAX_ERROR, loc);
}
//-----------------------------------------------------------------------------
bool
Prefix::is_value_parent(int pc) const
{
   // we have ) XXX with XXX on the stack and need to know if the evaluation
   // of (... ) will be a value as in e.g. (1 + 1) or a function as in (+/).
   //
   Assert1(body[pc].get_Class() == TC_R_PARENT);

   ++pc;
   if (pc >= body.size())   return true;   // syntax error

TokenClass next = body[pc].get_Class();

   if (next == TC_R_BRACK)   // skip [ ... ]
      {
        const int offset = body[pc].get_int_val2();
        pc += offset;
        Assert1(body[pc].get_Class() == TC_L_BRACK);   // opening [
        if (pc >= body.size())   return true;   // syntax error
        next = body[pc].get_Class();
      }

   if (next == TC_SYMBOL)   // resolve symbol if necessary
      {
        const Symbol * sym = body[pc].get_sym_ptr();
        const NameClass nc = sym->get_nc();
                      
        if (nc == NC_FUNCTION)   return false;
        if (nc == NC_OPERATOR)   return false;
        return true;
      }

   if (next == TC_OPER1)   return false;
   if (next == TC_OPER2)   return false;
   if (next == TC_FUN12)   return false;

   if (next == TC_L_PARENT)   // )) XXX
      {
        ++pc;
        if (!is_value_parent(pc))   return false;   // (fun)) XXX
        const int offset = body[pc].get_int_val();
        pc += offset;
        if (pc >= body.size())   return true;   // syntax error
        next = body[pc].get_Class();
        Assert1(next == TC_L_PARENT);   // opening (
        ++pc;
        if (pc >= body.size())   return true;   // syntax error

        //   (val)) XXX
        //  ^
        //  pc
        //
        // result is a value unless (val) is the right function operand
        // of a dyadic operator
        //
        next = body[pc].get_Class();
        if (next == TC_OPER2)   return false;
        if (next == TC_SYMBOL)   // resolve symbol if necessary
           {
             const Symbol * sym = body[pc].get_sym_ptr();
             const Function * fun = sym->get_function();
             return ! (fun && fun->is_operator() &&
                       fun->get_oper_valence() == 2);
           }
        return true;
      }

   return true;
}
//-----------------------------------------------------------------------------
bool
Prefix::is_value_bracket() const
{
   Assert1(body[PC - 1].get_Class() == TC_R_BRACK);
const int offset = body[PC - 1].get_int_val();
   Assert1(body[PC + offset - 1].get_Class() == TC_L_BRACK);   // opening [

const Token & tok1 = body[PC + offset];
   if (tok1.get_Class() == TC_VALUE)    return true;
   if (tok1.get_Class() != TC_SYMBOL)   return false;

Symbol * sym = tok1.get_sym_ptr();
const bool left_sym = get_assign_state() == ASS_arrow_seen;
   return sym->resolve_class(left_sym) == TC_VALUE;
}
//-----------------------------------------------------------------------------
int
Prefix::vector_ass_count() const
{
int count = 0;

   for (int pc = PC; pc < body.size(); ++pc)
       {
         if (body[pc].get_tag() != TOK_LSYMB2)   break;
         ++count;
       }

   return count;
}
//-----------------------------------------------------------------------------
void
Prefix::print_stack(ostream & out, const char * loc) const
{
const int si_depth = si.get_level();

   out << "fifo[si=" << si_depth << " len=" << size()
       << " PC=" << PC << "] is now :";

   loop(s, size())
      {
        const TokenClass tc = at(s).tok.get_Class();
        out << " " << Token::class_name(tc);
      }

   out << "  at " << loc << endl;
}
//-----------------------------------------------------------------------------
int
Prefix::show_owners(const char * prefix, ostream & out,
                          const Value & value) const
{
int count = 0;

   loop (s, size())
      {
        const Token & tok = at(s).tok;
        if (tok.get_ValueType() != TV_VAL)      continue;

        if (tok.get_apl_val()->is_or_contains(value))
           {
             out << prefix << " Fifo [" << s << "]" << endl;
             ++count;
           }

      }

   return count;
}
//-----------------------------------------------------------------------------
Function_PC
Prefix::get_range_high() const
{
   // if the stack is empty then return the last address (if any) or otherwise
   // the address of the next token.
   //
   if (size() == 0)     // stack is empty
      {
        if (lookahead_high == Function_PC_invalid)   return PC;
        return lookahead_high;
      }

   // stack non-empty: return address of highest item or the last address
   //
Function_PC high = lookahead_high;
   if (high == Function_PC_invalid)   high = at(0).pc;

   if (best && best->misc)   --high;
   return high;
}
//-----------------------------------------------------------------------------
Function_PC
Prefix::get_range_low() const
{
   // if the stack is not empty then return the PC of the lowest element
   //
   if (size() > 0)     return at(size() - 1).pc;


   // the stack is empty. Return the last address (if any) or otherwise
   // the address of the next token.
   //
   if (lookahead_high == Function_PC_invalid)   return PC;   // no last address
   return lookahead_high;
}
//-----------------------------------------------------------------------------
bool
Prefix::value_expected()
{
   // return true iff the current saved_lookahead token (which has been tested
   // to be a TC_INDEX token) is the index of a value and false it it is
   // a function axis

   // if it contains semicolons then get_ValueType() is TV_INDEX and
   // it MUST be a value.
   //
   if (saved_lookahead.tok.get_ValueType() == TV_INDEX)   return true;

   for (int pc = PC; pc < body.size();)
      {
        const Token & tok = body[pc++];
        switch(tok.get_Class())
           {
               case TC_R_BRACK:   // skip over [...] (func axis or value index)
                    //
                    pc += tok.get_int_val2();
                    continue;

               case TC_END:     return false;   // syntax error

               case TC_FUN0:    return true;   // niladic function is a value
               case TC_FUN12:   return false;  // function

               case TC_SYMBOL:
                    {
                      const Symbol * sym = tok.get_sym_ptr();
                      const NameClass nc = sym->get_nc();
                      
                      if (nc == NC_FUNCTION)   return false;
                      if (nc == NC_OPERATOR)   return false;
                      return true;   // value
                    }
             
               case TC_RETURN:  return false;   // syntax error
               case TC_VALUE:   return true;

               default: continue;
           }
      }

   // this is a syntax error.
   //
   return false;
}
//-----------------------------------------------------------------------------
void
Prefix::unmark_all_values() const
{
   loop (s, size())
      {
        const Token & tok = at(s).tok;
        if (tok.get_ValueType() != TV_VAL)      continue;

        Value_P value = tok.get_apl_val();
        if (!!value)   value->unmark();
      }
}
//-----------------------------------------------------------------------------

#define PH(name, idx, prio, misc, len, fun) \
   { #name, idx, prio, misc, len, &Prefix::reduce_ ## fun, #fun },

#include "Prefix.def"   // the hash table

Token
Prefix::reduce_statements()
{
   Log(LOG_prefix_parser)
      {
        CERR << endl << "changed to Prefix[si=" << si.get_level()
             << "]) ============================================" << endl;
      }

   if (size() > 0)   goto again;

grow:
   // the current stack does not contain a valid phrase.
   // Push one more token onto the stack and continue
   //
   {
     if (saved_lookahead.tok.get_tag() != TOK_VOID)
        {
          // there is a MISC token from a MISC phrase. Use it.
          //
          push(saved_lookahead);
          saved_lookahead.tok.clear(LOC);
          goto again;   // success
        }

     // if END was reached, then there are no more token in current-statement
     //
     if (size() > 0 && at0().get_Class() == TC_END)
        {
          Log(LOG_prefix_parser)   print_stack(CERR, LOC);
          syntax_error(LOC);   // no more token
        }

     Token_loc tl = lookahead();
     Log(LOG_prefix_parser)
        {
          CERR << "    [si=" << si.get_level() << " PC=" << (PC - 1)
               << "] Read token[" << size()
               << "] (←" << get_assign_state() << "←) " << tl.tok << " "
               << Token::class_name(tl.tok.get_Class()) << endl;
        }

     lookahead_high = tl.pc;
     TokenClass tcl = tl.tok.get_Class();

     if (tcl == TC_SYMBOL)   // resolve symbol if necessary
        {
          Symbol * sym = tl.tok.get_sym_ptr();
          if (tl.tok.get_tag() == TOK_LSYMB2)
             {
               // this is the last token C of a vector assignment
               // (A B ... C)←. We return C and let the caller do the rest
               //
               sym->resolve(tl.tok, true);
             }
          else
             {
               const bool left_sym = get_assign_state() == ASS_arrow_seen;
               sym->resolve(tl.tok, left_sym);
               if (left_sym)   set_assign_state(ASS_var_seen);
             }

          Log(LOG_prefix_parser)
             {
               CERR << "   resolved symbol " << sym->get_name()
                    << " to " << tl.tok.get_Class() << endl;
             }

          if (tl.tok.get_tag() == TOK_SI_PUSHED)
            {
              // Quad_Quad::resolve() calls ⍎ which returns TOK_SI_PUSHED.
              //
              push(tl);
              return Token(TOK_SI_PUSHED);
            }
        }
     else if (tcl == TC_ASSIGN)   // resolve symbol if necessary
        {
          if (get_assign_state() != ASS_none)   syntax_error(LOC);
          set_assign_state(ASS_arrow_seen);
        }

     push(tl);
   }

again:
   Log(LOG_prefix_parser)   print_stack(CERR, LOC);

   // search prefixes in phrase table...
   //
   {
     const int idx_0 = at0().get_Class();

     if (size() >= 3)
        {
          const int idx_01  = idx_0  | at1().get_Class() <<  5;
          const int idx_012 = idx_01 | at2().get_Class() << 10;

          if (size() >= 4)
             {
               const int idx_0123 = idx_012 | at3().get_Class() << 15;
               best = hash_table + idx_0123 % PHRASE_MODU;
               if (best->idx == idx_0123)   goto found_prefix;
             }

          best = hash_table + idx_012 % PHRASE_MODU;
          if (best->idx == idx_012)   goto found_prefix;

          best = hash_table + idx_01 % PHRASE_MODU;
          if (best->idx == idx_01)   goto found_prefix;

          best = hash_table + idx_0 % PHRASE_MODU;
          if (best->idx != idx_0)   goto grow;   // no matching phrase
        }
     else   // 0 < size() < 3
        {
          if (size() >= 2)
             {
               const int idx_01 = idx_0 | at1().get_Class() << 5;
               best = hash_table + idx_01 % PHRASE_MODU;
               if (best->idx == idx_01)   goto found_prefix;
             }

          best = hash_table + idx_0 % PHRASE_MODU;
          if (best->idx != idx_0)   goto grow;
        }
   }

found_prefix:

   // found a reducible prefix. See if the next token class binds stronger
   // than best->prio
   //
   {
     TokenClass next = TC_INVALID;
     if (PC < body.size())
        {
          const Token & tok = body[PC];

          next = tok.get_Class();
          if (next == TC_SYMBOL)
             {
               Symbol * sym = tok.get_sym_ptr();
               const bool left_sym = get_assign_state() == ASS_arrow_seen;
               next = sym->resolve_class(left_sym);
            }
        }

     if (best->misc && (at0().get_Class() == TC_R_BRACK))
        {
          // the next symbol is a ] and the matching phrase is a MISC
          // phrase (monadic call of a possibly dyadic function).
          // The ] could belong to:
          //
          // 1. an indexed value,        e.g. A[X] or
          // 2. a function with an axis, e.g. +[2] 
          //
          // These cases lead to different reductions:
          //
          // 1.  A[X] × B   should evalate × dyadically, while
          // 2.  +[1] × B   should evalate × monadically,
          //
          // We solve this by computing the indexed value first
          //
          if (is_value_bracket())   // case 1.
             {
               // we call reduce_RBRA____, which pushes a partial index list
               // onto the stack. The following token are processed until the
               // entire indexed value A[ ... ] is computed
               prefix_len = 1;
               reduce_RBRA___();
               goto grow;
             }
        }

//   Q(next) Q(at0())

     // we could reduce, but we could also shift. Compute more, which is true
     // if we should shift.
     //
     const bool shift = dont_reduce(next);
     if (shift)
        {
           Log(LOG_prefix_parser)  CERR
               << "   phrase #" << (best - hash_table)
               << ": " << best->phrase_name
               << " matches, but prio " << best->prio
               << " is too small to call " << best->reduce_name
               << "()" << endl;
          goto grow;
        }
   }

   Log(LOG_prefix_parser)  CERR
      << "   phrase #" <<  (best - hash_table)
      << ": " << best->phrase_name
      << " matches, prio " << best->prio
      << ", calling reduce_" << best->reduce_name
      << "()" << endl;

   action = RA_FIXME;
   prefix_len = best->phrase_len;
   if (best->misc)   // MISC phrase: save X and remove it
      {
        Assert(saved_lookahead.tok.get_tag() == TOK_VOID);
        saved_lookahead.copy(pop(), LOC);
        --prefix_len;
      }

   (this->*best->reduce_fun)();

   Log(LOG_prefix_parser)
      CERR << "   reduce_" << best->reduce_name << "() returned: ";

   // handle action (with decreasing likelihood)
   //
   if (action == RA_CONTINUE)
      {
        Log(LOG_prefix_parser)   CERR << "RA_CONTINUE" << endl;
        goto again;
      }

   if (action == RA_PUSH_NEXT)
      {
        Log(LOG_prefix_parser)   CERR << "RA_PUSH_NEXT" << endl;
        goto grow;
      }

   if (action == RA_SI_PUSHED)
      {
        Log(LOG_prefix_parser)   CERR << "RA_SI_PUSHED" << endl;
        return Token(TOK_SI_PUSHED);
      }

   if (action == RA_RETURN)
      {
        Log(LOG_prefix_parser)   CERR << "RA_RETURN" << endl;
        return pop().tok;
      }

   if (action == RA_FIXME)
      {
        Log(LOG_prefix_parser)   CERR << "RA_FIXME" << endl;
        FIXME;
      }

   FIXME;
}
//-----------------------------------------------------------------------------
bool
Prefix::dont_reduce(TokenClass next) const
{
   if (at0().get_Class() == TC_VALUE)
      {
        if (next == TC_OPER2)           // DOP B
           {
             return best->prio < BS_OP_RO;
           }
        else if (next == TC_VALUE)      // A B
           {
             return best->prio < BS_VAL_VAL;
           }
        else if (next == TC_R_PARENT)   // ) B
           {
             if (is_value_parent(PC))     // e.g. (X+Y) B
                {
                  return best->prio < BS_VAL_VAL;
                }
              else                      // e.g. (+/) B
                {
                  return false;
                }
           }
      }
   else if (at0().get_Class() == TC_FUN12)
      {
        if (next == TC_OPER2)
           {
             return best->prio < BS_OP_RO;
           }
      }

   return false;
}
//-----------------------------------------------------------------------------
bool
Prefix::replace_AB(Value_P old_value, Value_P new_value)
{
   Assert(old_value);
   Assert(new_value);

   loop(s, size())
     {
       Token & tok = at(s).tok;
       if (tok.get_Class() != TC_VALUE)   continue;
       if (tok.get_apl_val() == old_value)   // found
          {
            new (&tok) Token(tok.get_tag(), new_value);
            return true;
          }
     }
   return false;
}
//-----------------------------------------------------------------------------
Token * Prefix::locate_L()
{
   // expect at least A f B (so we have at0(), at1() and at2()

   if (prefix_len < 3)   return 0;

   if (at1().get_Class() != TC_FUN12 &&
       at1().get_Class() != TC_OPER1 &&
       at1().get_Class() != TC_OPER2)   return 0;

   if (at0().get_Class() == TC_VALUE)   return &at0();
   return 0;
}
//-----------------------------------------------------------------------------
Value_P *
Prefix::locate_X()
{
   // expect at least B X (so we have at0() and at1() and at2()

   if (prefix_len < 2)   return 0;

   // either at0() (for monadic f X B) or at1() (for dyadic A f X B) must
   // be a function or operator
   //
   for (int x = put - 1; x >= put - prefix_len; --x)
       {
         if (content[x].tok.get_ValueType() == TV_FUN)
            {
              Function * fun = content[x].tok.get_function();
              if (fun)
                 {
                   Value_P * X = fun->locate_X();
                   if (X)   return  X;   // only for derived function
                 }
            }
         else if (content[x].tok.get_Class() == TC_INDEX)   // maybe found X ?
            {
              return content[x].tok.get_apl_valp();
            }
       }

   return 0;
}
//-----------------------------------------------------------------------------
Token * Prefix::locate_R()
{
   // expect at least f B (so we have at0(), at1() and at2()

   if (prefix_len < 2)   return 0;

   // either at0() (for monadic f B) or at1() (for dyadic A f B) must
   // be a function or operator
   //
   if (at0().get_Class() != TC_FUN12 &&
       at0().get_Class() != TC_OPER1 &&
       at0().get_Class() != TC_OPER2 &&
       at1().get_Class() != TC_FUN12 &&
       at1().get_Class() != TC_OPER1 &&
       at1().get_Class() != TC_OPER2)   return 0;

Token * ret = &content[put - prefix_len].tok;
   if (ret->get_Class() == TC_VALUE)   return ret;
   return 0;
}
//-----------------------------------------------------------------------------
void
Prefix::print(ostream & out, int indent) const
{
   loop(i, indent)   out << "    ";
   out << "Token: ";
   loop(s, size())   out << " " << at(s).tok;
   out << endl;
}
//=============================================================================
//
// phrase reduce functions...
//
//-----------------------------------------------------------------------------
void
Prefix::reduce____()
{
   // this function is a placeholder for invalid phrases and should never be
   // called.
   //
   FIXME;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_LPAR_B_RPAR_()
{
   Assert1(prefix_len == 3);

Token result = at1();   // B or F
   if (result.get_tag() == TOK_APL_VALUE3)   result.ChangeTag(TOK_APL_VALUE1);
   pop_args_push_result(result);
   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_LPAR_F_RPAR_()
{
   reduce_LPAR_B_RPAR_();   // same as ( B )
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_LPAR_F_C_RPAR()
{
   Assert1(prefix_len == 4);

   // C should be an axis and not a [;;] index
   //
   if (at2().get_ValueType() != TV_VAL)   SYNTAX_ERROR;
   if (!at2().get_apl_val())              SYNTAX_ERROR;

   move_1(at3(), at2(), LOC);
   move_1(at2(), at1(), LOC);
   pop();    // pop C
   pop();    // pop RPAR
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_N___()
{
   Assert1(prefix_len == 1);

Token result = at0().get_function()->eval_();
   if (result.get_tag() == TOK_ERROR)
      {
        Token_loc tl(result, get_range_low());
        push(tl);
        action = RA_RETURN;
        return;
      }

   pop_args_push_result(result);
   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_MISC_F_B_()
{
   Assert1(prefix_len == 2);

   if (saved_lookahead.tok.get_Class() == TC_INDEX)
      {
        if (value_expected())
           {
             // push [...] and read one more token
             //
             push(saved_lookahead);
             saved_lookahead.tok.clear(LOC);
             action = RA_PUSH_NEXT;
             return;
           }
      }

Token result = at0().get_function()->eval_B(at1().get_apl_val());
   if (result.get_tag() == TOK_ERROR)
      {
        Token_loc tl(result, get_range_low());
        push(tl);
        action = RA_RETURN;
        return;
      }

   pop_args_push_result(result);
   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_MISC_F_C_B()
{
   Assert1(prefix_len == 3);

   if (saved_lookahead.tok.get_Class() == TC_INDEX)
      {
        if (value_expected())
           {
             // push [...] and read one more token
             //
             push(saved_lookahead);
             saved_lookahead.tok.clear(LOC);
             action = RA_PUSH_NEXT;
             return;
           }
      }

   if (at1().get_ValueType() != TV_VAL)   // [i1;i2...] instead of [axis]
      {
        IndexExpr * idx = &at1().get_index_val();
        Log(LOG_delete)   CERR << "delete " HEX(idx) << " at " LOC << endl;
        delete idx;
         at1().clear(LOC);
         SYNTAX_ERROR;
      }
   if (!at1().get_apl_val())              SYNTAX_ERROR;

Token result = at0().get_function()->eval_XB(at1().get_apl_val(),
                                             at2().get_apl_val());
   if (result.get_tag() == TOK_ERROR)
      {
        Token_loc tl(result, get_range_low());
        push(tl);
        action = RA_RETURN;
        return;
      }

   pop_args_push_result(result);
   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_A_F_B_()
{
   Assert1(prefix_len == 3);

Token result = at1().get_function()->eval_AB(at0().get_apl_val(),
                                             at2().get_apl_val());
   if (result.get_tag() == TOK_ERROR)
      {
        Token_loc tl(result, get_range_low());
        push(tl);
        action = RA_RETURN;
        return;
      }

   pop_args_push_result(result);
   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_A_M_B_()
{
   Assert1(prefix_len == 3);

   reduce_A_F_B_();
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_A_F_C_B()
{
   Assert1(prefix_len == 4);

   if (at2().get_ValueType() != TV_VAL)   SYNTAX_ERROR;
   if (!at2().get_apl_val())              SYNTAX_ERROR;

Token result = at1().get_function()->eval_AXB(at0().get_apl_val(),
                                              at2().get_apl_val(),
                                              at3().get_apl_val());
   if (result.get_tag() == TOK_ERROR)
      {
        Token_loc tl(result, get_range_low());
        push(tl);
        action = RA_RETURN;
        return;
      }

   pop_args_push_result(result);
   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_A_M_C_B()
{
   reduce_A_F_C_B();
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_F_M__()
{
DerivedFunction * derived =
   Workspace::SI_top()->fun_oper_cache.get(LOC);
   new (derived) DerivedFunction(at0(), at1().get_function(), LOC);

   pop_args_push_result(Token(TOK_FUN2, derived));
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_F_M_C_()
{
DerivedFunction * derived =
   Workspace::SI_top()->fun_oper_cache.get(LOC);
   new (derived) DerivedFunction(at0(), at1().get_function(),
                                 at2().get_axes(), LOC);

   pop_args_push_result(Token(TOK_FUN2, derived));
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_B_D_B_()
{
   reduce_F_D_G_();   // same as F2
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_B_D_G_()
{
   reduce_F_D_G_();   // same as F2
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_F_D_B_()
{
   // same as G2, except for ⍤
   //
   if (at1().get_function()->get_Id() != ID::OPER2_RANK)
      {
         reduce_F_D_G_();
         return;
      }

   // we have f ⍤ y_B with y_B glued beforehand. Unglue it.
   //
Value_P y123;
Value_P B;
   Bif_OPER2_RANK::split_y123_B(at2().get_apl_val(), y123, B);
Token new_y123(TOK_APL_VALUE1, y123);

DerivedFunction * derived = Workspace::SI_top()->fun_oper_cache.get(LOC);
   new (derived) DerivedFunction(at0(), at1().get_function(), new_y123, LOC);

Token result = Token(TOK_FUN2, derived);

   if (!B)   // only y123, no B (e.g. (f ⍤[X] 1 2 3)
      {
        pop_args_push_result(result);
      }
   else      // a new B split off from the original B
      {
        // save locations of ⍤ and B
        //
        Function_PC pc_D = at(1).pc;
        Function_PC pc_B = at(3).pc;

        pop_and_discard();   // pop F
        pop_and_discard();   // pop C
        pop_and_discard();   // pop B (old)

        Token new_B(TOK_APL_VALUE1, B);
        Token_loc tl_B(new_B, pc_B);
        Token_loc tl_derived(result, pc_D);
        push(tl_B);
        push(tl_derived);
      }

   action = RA_CONTINUE;

}
//-----------------------------------------------------------------------------
void
Prefix::reduce_F_D_G_()
{
   // reduce, except if another dyadic operator is coming. In that case
   // F belongs to the other operator and we simply continue.
   //
   if (PC < body.size())
        {
          const Token & tok = body[PC];
          TokenClass next =  tok.get_Class();
          if (next == TC_SYMBOL)
             {
               Symbol * sym = tok.get_sym_ptr();
               const bool left_sym = get_assign_state() == ASS_arrow_seen;
               next = sym->resolve_class(left_sym);
             }

          if (next == TC_OPER2)
             {
               action = RA_PUSH_NEXT;
               return;
             }
        }

DerivedFunction * derived =
   Workspace::SI_top()->fun_oper_cache.get(LOC);
   new (derived) DerivedFunction(at0(), at1().get_function(), at2(), LOC);

   pop_args_push_result(Token(TOK_FUN2, derived));
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_F_D_C_B()
{
   // reduce, except if another dyadic operator is coming. In that case
   // F belongs to the other operator and we simply continue.
   //
   if (PC < body.size())
        {
          const Token & tok = body[PC];
          TokenClass next =  tok.get_Class();
          if (next == TC_SYMBOL)
             {
               Symbol * sym = tok.get_sym_ptr();
               const bool left_sym = get_assign_state() == ASS_arrow_seen;
               next = sym->resolve_class(left_sym);
             }

          if (next == TC_OPER2)
             {
               action = RA_PUSH_NEXT;
               return;
             }
        }
   // we have f ⍤ [X] y_B with y_B glued beforehand. Unglue it.
   //
Value_P y123;
Value_P B;
   Bif_OPER2_RANK::split_y123_B(at3().get_apl_val(), y123, B);
Token new_y123(TOK_APL_VALUE1, y123);

Value_P v_idx = at2().get_axes();
DerivedFunction * derived = Workspace::SI_top()->fun_oper_cache.get(LOC);
   new (derived) DerivedFunction(at0(), at1().get_function(),
                                 v_idx, new_y123, LOC);

Token result = Token(TOK_FUN2, derived);

   if (!B)   // only y123, no B (e.g. (f ⍤[X] 1 2 3)
      {
        pop_args_push_result(result);
      }
   else      // a new B split off from the original B
      {
        // save locations of ⍤ and B
        //
        Function_PC pc_D = at(1).pc;
        Function_PC pc_B = at(3).pc;

        pop_and_discard();   // pop F
        pop_and_discard();   // pop D
        pop_and_discard();   // pop C
        pop_and_discard();   // pop B (old)

        Token new_B(TOK_APL_VALUE1, B);
        Token_loc tl_B(new_B, pc_B);
        Token_loc tl_derived(result, pc_D);
        push(tl_B);   
        push(tl_derived);   
      }

   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_A_C__()
{
Value_P A = at0().get_apl_val();
Value_P Z;

   if (at1().get_tag() == TOK_AXES)
      {
        Z = A->index(at1().get_apl_val());
      }
   else
      {
        IndexExpr * idx =  &at1().get_index_val();
        try
           {
             Z = A->index(*idx);
             Log(LOG_delete)   CERR << "delete " HEX(idx) << " at " LOC << endl;
             delete idx;
           }
        catch (Error err)
           {
             Token result = Token(TOK_ERROR, err.error_code);
             Log(LOG_delete)   CERR << "delete " HEX(idx) << " at " LOC << endl;
             delete idx;
             pop_args_push_result(result);
             set_action(result);
             return;
           }
      }

Token result = Token(TOK_APL_VALUE1, Z);
   pop_args_push_result(result);

   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_V_C_ASS_B()
{
Symbol * V = at0().get_sym_ptr();
Value_P B = at3().get_apl_val();

   if (at1().get_tag() == TOK_AXES)   // [] or [x]
      {
        Value_P v_idx = at1().get_axes();

        try
           {
             V->assign_indexed(v_idx, B);
           }
        catch (Error err)
           {
             Token result = Token(TOK_ERROR, err.error_code);
             at1().clear(LOC);
             at3().clear(LOC);
             pop_args_push_result(result);
             set_assign_state(ASS_none);
             set_action(result);
             return;
           }
      }
   else                               // [a;...]
      {
        IndexExpr * idx = &at1().get_index_val();
        try
           {
             V->assign_indexed(*idx, B);
             Log(LOG_delete)   CERR << "delete " HEX(idx) << " at " LOC << endl;
             delete idx;
           }
        catch (Error err)
           {
             Token result = Token(TOK_ERROR, err.error_code);
             at1().clear(LOC);
             at3().clear(LOC);
             Log(LOG_delete)   CERR << "delete " HEX(idx) << " at " LOC << endl;
             delete idx;
             pop_args_push_result(result);
             set_assign_state(ASS_none);
             set_action(result);
             return;
           }
      }

Token result = Token(TOK_APL_VALUE2, B);
   pop_args_push_result(result);
   set_assign_state(ASS_none);
   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_F_V__()
{
   // turn V into a (left-) value
   //
Symbol * V = at1().get_sym_ptr();
   copy_1(at1(), V->resolve_lv(LOC), LOC);
   set_assign_state(ASS_var_seen);
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_M_V__()
{
   reduce_F_V__();   // same as F2
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_A_ASS_B_()
{
Value_P A = at0().get_apl_val();
Value_P B = at2().get_apl_val();

   A->assign_cellrefs(B);   // erases A !

Token result = Token(TOK_APL_VALUE2, B);
   pop_args_push_result(result);

   set_assign_state(ASS_none);
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_V_ASS_B_()
{
Value_P B = at2().get_apl_val();
Symbol * V = at0().get_sym_ptr();
   V->assign(B, LOC);

Token result = Token(TOK_APL_VALUE2, B);
   pop_args_push_result(result);

   set_assign_state(ASS_none);
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_V_ASS_F_()
{
   // named lambda: V ← { ... }
   //
Function * F = at2().get_function();
   if (!F->is_lambda())   SYNTAX_ERROR;

Symbol * V = at0().get_sym_ptr();
   V->assign_named_lambda(F, LOC);

Value_P Z(V->get_name(), LOC);
Token result = Token(TOK_APL_VALUE2, Z);
   pop_args_push_result(result);

   set_assign_state(ASS_none);
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_V_ASS_N_()
{
   // named lambda: V ← { ... } niladic (same as reduce_V_ASS_F_)
   //
   reduce_V_ASS_F_();
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_V_ASS_M_()
{
   // named lambda: V ← { ... } monadic operator (same as reduce_V_ASS_F_)
   //
   reduce_V_ASS_F_();
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_V_ASS_D_()
{
   // named lambda: V ← { ... } dyadic operator (same as reduce_V_ASS_F_)
   //
   reduce_V_ASS_F_();
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_RBRA___()
{
   Assert1(prefix_len == 1);

   // start partial index list. Parse the index as right so that, for example,
   // A[IDX}←B resolves IDX properly. assign_state is restored when the
   // index is complete.
   //
IndexExpr * idx = new IndexExpr(get_assign_state(), LOC);
   Log(LOG_delete)
      CERR << "new    " << (const void *)idx << " at " LOC << endl;

   new (&at0()) Token(TOK_PINDEX, *idx);
   set_assign_state(ASS_none);
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_LBRA_I__()
{
   // [ I or ; I   (elided index)
   //
   Assert1(prefix_len == 2);

IndexExpr & idx = at1().get_index_val();
const bool last_index = (at0().get_tag() == TOK_L_BRACK);

   if (idx.value_count() == 0 && last_index)   // special case: [ ]
      {
        assign_state = idx.get_assign_state();
        Token result = Token(TOK_INDEX, idx);
        pop_args_push_result(result);
        action = RA_CONTINUE;
        Log(LOG_delete)   CERR << "delete " HEX(&idx) << " at " LOC << endl;
        delete &idx;
        return;
      }

   // add elided index to partial index list
   //
Token result = at1();
   result.get_index_val().add(Value_P());

   if (last_index)   // [ seen
      {
        assign_state = idx.get_assign_state();

        if (idx.is_axis()) move_2(result, Token(TOK_AXES, idx.values[0]), LOC);
        else               move_2(result, Token(TOK_INDEX, idx), LOC);
      }
   else
      {
        set_assign_state(ASS_none);
      }

   pop_args_push_result(result);
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_LBRA_B_I_()
{
   Assert1(prefix_len == 3);

   // [ B I or ; B I   (normal index)
   //
Token I = at2();
   I.get_index_val().add(at1().get_apl_val());

const bool last_index = (at0().get_tag() == TOK_L_BRACK);   // ; vs. [

   if (last_index)   // [ seen
      {
        IndexExpr & idx = I.get_index_val();
        assign_state = idx.get_assign_state();

        if (idx.is_axis())
           {
             Value_P X = idx.extract_value(0);
             Assert1(!!X);
             move_2(I, Token(TOK_AXES, X), LOC);
             Log(LOG_delete)
                CERR << "delete " HEX(&idx) << " at " LOC << endl;
             delete &idx;
           }
        else
           {
             move_2(I, Token(TOK_INDEX, idx), LOC);
           }
      }
   else
      {
         set_assign_state(ASS_none);
      }

   pop_args_push_result(I);
   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_A_B__()
{
   Assert1(prefix_len == 2);

Token result;
   Value::glue(result, at0(), at1(), LOC);
   pop_args_push_result(result);

   set_action(result);
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_V_RPAR_ASS_B()
{
   Assert1(prefix_len == 4);

const int count = vector_ass_count();

   // vector assignment also matches selective specification, but count
   // distinguishes them, e.g.
   //
   // (T U V) ← value        count = 2	    vector assignment
   //   (U V) ← value        count = 1	    vector assignment
   // (2 ↑ V) ← value        count = 0	    selective specification
   //
   if (count < 1)
      {
        // selective specification. Convert variable V into a (left-) value
        //
        Symbol * V = at0().get_sym_ptr();
        Token result = V->resolve_lv(LOC);
        set_assign_state(ASS_var_seen);
        move_1(at0(), result, LOC);
        action = RA_CONTINUE;
        return;
      }

   // vector assignment.
   //
DynArray(Symbol *, symbols, count + 1);
   symbols[0] = at0().get_sym_ptr();
   loop(c, count)
      {
        Token_loc tl = lookahead();
        Assert1(tl.tok.get_tag() == TOK_LSYMB2);   // by vector_ass_count()
        Symbol * V = tl.tok.get_sym_ptr();
        Assert(V);
        symbols[c + 1] = V;
      }

Value_P B = at3().get_apl_val();
   Symbol::vector_assignment(symbols, count + 1, B);

   set_assign_state(ASS_none);

   // clean up stack
   //
Token result(TOK_APL_VALUE2, at3().get_apl_val());
   pop_args_push_result(result);
Token_loc tl = lookahead();
   if (tl.tok.get_Class() != TC_L_PARENT)   syntax_error(LOC);

   action = RA_CONTINUE;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_END_VOID__()
{
   Assert1(prefix_len == 2);

   if (size() != 2)   syntax_error(LOC);

const bool end_of_line = at0().get_tag() == TOK_ENDL;
const bool trace = (at0().get_int_val() & 1) != 0;

   pop_and_discard();   // pop END
   pop_and_discard();   // pop VOID

Token Void(TOK_VOID);
   si.statement_result(Void, trace);
   action = RA_PUSH_NEXT;
   if (attention_raised && end_of_line)
      {
        attention_raised = false;
        interrupt_raised = false;
        ATTENTION;
      }
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_END_B__()
{
   Assert1(prefix_len == 2);

   if (size() != 2)   syntax_error(LOC);

const bool end_of_line = at0().get_tag() == TOK_ENDL;
const bool trace = (at0().get_int_val() & 1) != 0;

   pop_and_discard();   // pop END
Token B = pop().tok;    // pop B
   si.statement_result(B, trace);

   action = RA_PUSH_NEXT;
   if (attention_raised && end_of_line)
      {
        attention_raised = false;
        interrupt_raised = false;
        ATTENTION;
      }
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_END_GOTO_B_()
{
   Assert1(prefix_len == 3);

   if (size() != 3)   syntax_error(LOC);

   // at0() is either TOK_END or TOK_ENDL.
   //
const bool end_of_line = at0().get_tag() == TOK_ENDL;
const bool trace = at0().get_Class() == TC_END &&
                  (at0().get_int_val() & 1) != 0;

Value_P line = at2().get_apl_val();
   if (trace && line->element_count() > 0)
      {
        const int64_t line_num = line->get_line_number();
        Token bra(TOK_BRANCH, line_num);
        si.statement_result(bra, true);
      }

const Token result = si.jump(line);

   if (result.get_tag() == TOK_BRANCH)   // branch back into a function
      {
        Log(LOG_prefix_parser)
           {
             CERR << "Leaving context after " << result << endl;
           }

        pop_args_push_result(result);
        action = RA_RETURN;
        return;
      }

   // StateIndicator::jump may have called set_PC() which resets the prefix.
   // we do not call pop_args_push_result(result) (which would fail due
   // to the now incorrect prefix_len), but discard the entire statement.
   //
   reset(LOC);

   if (result.get_tag() == TOK_NOBRANCH)   // branch not taken, e.g. →⍬
      {
        if (trace)
           {
             Token bra(TOK_NOBRANCH);
             si.statement_result(bra, true);
           }

        action = RA_PUSH_NEXT;
        if (attention_raised && end_of_line)
           {
             attention_raised = false;
             interrupt_raised = false;
             ATTENTION;
           }

        return;
      }

   if (result.get_tag() == TOK_VOID)   // branch taken, e.g. →N
      {
        action = RA_PUSH_NEXT;
        if (attention_raised && end_of_line)
           {
             attention_raised = false;
             interrupt_raised = false;
             ATTENTION;
           }

        return;
      }

   // branch within function
   //
const Function_PC new_pc = si.get_PC();
   Log(LOG_prefix_parser)
      {
        CERR << "Staying in context after →PC(" << new_pc << ")" << endl;
        print_stack(CERR, LOC);
      }

   Assert1(size() == 0);
   action = RA_PUSH_NEXT;
   if (attention_raised && end_of_line)
      {
        attention_raised = false;
        interrupt_raised = false;
        ATTENTION;
      }
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_END_GOTO__()
{
   Assert1(prefix_len == 2);

   if (size() != 2)   syntax_error(LOC);

const bool trace = at0().get_Class() == TC_END &&
                  (at0().get_int_val() & 1) != 0;
   if (trace)
      {
        Token bra(TOK_ESCAPE);
        si.statement_result(bra, true);
      }

   // the statement is → which could mean TOK_ESCAPE (normal →) or
   //  TOK_STOP_LINE from S∆←line
   //
   if (at1().get_tag() == TOK_STOP_LINE)   // S∆ line
      {
        const UserFunction * ufun = si.get_executable()->get_ufun();
        if (ufun && ufun->get_exec_properties()[2])
           {
              // the function ignores attention (aka. weak interrupt)
              //
              pop_and_discard();   // pop() END
              pop_and_discard();   // pop() GOTO
              action = RA_CONTINUE;
              return;
           }

        COUT << si.function_name() << "[" << si.get_line() << "]" << endl;
        Token result(TOK_ERROR, E_STOP_LINE);
        pop_args_push_result(result);
        action = RA_RETURN;
      }
   else
      {
        Token result(TOK_ESCAPE);
        pop_args_push_result(result);
        action = RA_RETURN;
      }
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_RETC_VOID__()
{
   Assert1(prefix_len == 2);

Token result = Token(TOK_VOID);
   pop_args_push_result(result);
   action = RA_RETURN;
}
//-----------------------------------------------------------------------------
void
Prefix::reduce_RETC___()
{
   Assert1(prefix_len == 1);

   if (size() != 1)   syntax_error(LOC);

   // end of context reached. There are 4 cases:
   //
   // TOK_RETURN_STATS:  end of ◊ context
   // TOK_RETURN_EXEC:   end of ⍎ context with no result (e.g. ⍎'')
   // TOK_RETURN_VOID:   end of ∇ (no result)
   // TOK_RETURN_SYMBOL: end of ∇ (result in Z)
   //
   // case TOK_RETURN_EXEC (end of ⍎ context) is handled in reduce_RETC_B___()
   //
   switch(at0().get_tag())
      {
        case TOK_RETURN_EXEC:   // immediate execution context
             Log(LOG_prefix_parser)
                CERR << "- end of ⍎ context (no result)" << endl;
             at0().clear(LOC);
             action = RA_RETURN;
             return;

        case TOK_RETURN_STATS:   // immediate execution context
             Log(LOG_prefix_parser)
                CERR << "- end of ◊ context" << endl;
             at0().clear(LOC);
             action = RA_RETURN;
             return;

        case TOK_RETURN_VOID:   // user-defined function not returning a value
             Log(LOG_prefix_parser)
                CERR << "- end of ∇ context (function has no result)" << endl;

             {
               const UserFunction * ufun = si.get_executable()->get_ufun();
               Assert1(ufun);
               at0().clear(LOC);
             }
             action = RA_RETURN;
             return;

        case TOK_RETURN_SYMBOL:   // user-defined function returning a value
             {
               const UserFunction * ufun = si.get_executable()->get_ufun();
               Assert1(ufun);
               Symbol * ufun_Z = ufun->get_sym_Z();
               Value_P Z;
               if (ufun_Z)   Z = ufun_Z->get_value();
               if (!!Z)
                  {
                    Log(LOG_prefix_parser)
                       CERR << "- end of ∇ context (function result is: "
                            << *Z << ")" << endl;
                    new (&at0()) Token(TOK_APL_VALUE1, Z);
                  }
               else
                  {
                    Log(LOG_prefix_parser)
                       CERR << "- end of ∇ context (MISSING function result)."
                            << endl;
                    at0().clear(LOC);
                  }
             }
             action = RA_RETURN;
             return;

        default: break;
      }

   // not reached
   //
   Q1(at0().get_tag())   FIXME;
}
//-----------------------------------------------------------------------------
// Note: reduce_RETC_B___ happens only for context ⍎,
//       since contexts ◊ and ∇ use reduce_END_B___ instead.
//
void
Prefix::reduce_RETC_B__()
{
   Assert1(prefix_len == 2);

   if (size() != 2)
      {
        syntax_error(LOC);
      }

   Log(LOG_prefix_parser)
      CERR << "- end of ⍎ context.";

Token B = at1();
   pop_args_push_result(B);

   action = RA_RETURN;
}
//-----------------------------------------------------------------------------
// Note: reduce_RETC_GOTO_B__ happens only for context ⍎,
//       since contexts ◊ and ∇ use reduce_END_GOTO_B__ instead.
//
void
Prefix::reduce_RETC_GOTO_B_()
{
   if (size() != 3)   syntax_error(LOC);

   reduce_END_GOTO_B_();

   if (action == RA_PUSH_NEXT)
      {
        // reduce_END_GOTO_B_() has detected a non-taken branch (e.g. →'')
        // and wants to continue with the next statement.
        // We are in ⍎ mode, so there is no next statement and we
        // RA_RETURN a TOK_VOID instead of RA_PUSH_NEXT.
        //
        Token tok(TOK_VOID);
        Token_loc tl(tok, Function_PC_0);
        push(tl);
      }

   action = RA_RETURN;
}
//-----------------------------------------------------------------------------
// Note: reduce_RETC_ESC___ happens only for context ⍎,
//       since contexts ◊ and ∇ use reduce_END_ESC___ instead.
//
void
Prefix::reduce_RETC_GOTO__()
{
   if (size() != 2)   syntax_error(LOC);

   reduce_END_GOTO__();
}
//-----------------------------------------------------------------------------

