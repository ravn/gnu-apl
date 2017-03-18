%% -*- coding: utf-8 -*-
%%
%%  This file is part of GNU APL, a free implementation of the
%%  ISO/IEC Standard 13751, "Programming Language APL, Extended"
%%
%%  Copyright (C) 2008-2017  Dr. Jürgen Sauermann
%%
%%  This program is free software: you can redistribute it and/or modify
%%  it under the terms of the GNU General Public License as published
%%  by the Free Software Foundation, either version 3 of the License, or
%%  (at your option) any later version.
%%
%%  This program is distributed in the hope that it will be useful,
%%  but WITHOUT ANY WARRANTY; without even the implied warranty of
%%  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
%%  GNU General Public License for more details.
%%
%%  You should have received a copy of the GNU General Public License
%%  along with this program.  If not, see <http://www.gnu.org/licenses/>.
%%

-module(apl_example).
-export([start/0]).

start() ->
   io:format("starting apl_example...~n"),
  apl:init(),
   io:format("module apl initialized...~n"),
  example(),
  ok.

% an example of using APL from Erlang
example() ->
  % create a 4 by 4 matrix with numbers 1, 2, ... and assign it to Z
  io:format("~n~n~nSet APL variable Z to a 4×4 matrix...~n"),
  io:format("~p~n", apl:statement("Z←4 4⍴⍳99")),

  % create a 4 by 4 matrix with numbers 1, 2, ... and display it
  io:format("~nDisplay a 4×4 matrix...~n"),
  io:format("~p~n", [apl:statement("4 4⍴⍳99")]),

  % create a 3 by 3 matrix with numbers 1, 2, ... and assign it to Z
  io:format("~nSet Z to a 3×3 matrix... ~n"),
  io:format("~p~n", [apl:statement("Z←3 3⍴⍳99 ◊ Z")]),

  % provoke a syntax error
  io:format("~nProvoke a syntax error...~n"),
  io:format("~w~n", [apl:statement("Z++")]),

  % show APL variables
  io:format("~nShow all APL variables...~n"),
  io:format("~ts~n", [apl:command(")VARS")]),

  % create an APL functions named FOO and TIME
  io:format("~nCreate (aka. fix) APL function FOO...~n"),
  io:format("~w~n", [fix_FOO()]),
  io:format("~w~n", [fix_TIME()]),

  % show all APL functions
  io:format("~nShow all APL functions...~n"),
  io:format("~ts~n", [apl:command(")FNS")]),

  % display APL function FOO (a 2 line by 7 columns character matrix)
  io:format("~ndisplay APL function FOO...~n"),
  io:format("~tp~n", [apl:statement("⎕CR 'FOO'")]),

  % create a 10 by 30 integer matrix of 0s and initialize the first
  % few elements of it from an Erlang value with integers floats, and complex
  io:format("~nSet variabe Y directly from Erlang...~n"
            "NOTE: missing items are set to 0~n"),
  io:format("~s~n", [apl:set_variable("Y", [10,30],
                        [1, 2.2, {complex,3,4.4},{value,[3],"BAR"}])]),

  % display Y
  io:format("~nDisplay Y...~n"),
  io:format("~w~n", [apl:statement("Y")]),

  % create a 5 element numeric vector V and then set its middle argument to a
  % (nested) character string "Hello"
  io:format("~nSetup nested APL value V...~n"),
  io:format("~tp~n", [apl:statement("V←⍳5 ◊ V[3]←⊂'Nested'")]),

  % display V
  io:format("~nDisplay V...~n"),
  io:format("~w~n", [apl:statement("V")]),

   % call monadic APL function with Erlang term [1 2 3]
  io:format("~ncall monadic APL function '○' (aka. Pi times) with Erlang "
            " term [1, 2.2, 3.3]...~n"),
  io:format("result: ~tp~n", [apl:eval_B("○", apl:e2a([1, 2.2, 3.3]))]),
   
   % call dyadic APL function '+' with Erlang terms [1 2 3] and [4 5 6]
  io:format("~ncall dyadic APL function '+' with Erlang"
            " terms [1,2,3] and [4,5,6]...~n"),
  io:format("result: ~tp~n", [apl:eval_AB(apl:e2a([1,2,3]), "+",
                                          apl:e2a([4,5,6]))]),
   
   % call monadic APL operator '+/' with Erlang term [1 2 3 4 5 6]
  io:format("~ncall monadic APL operator '+/' with Erlang"
            " term [1,2,3,4,5,6]...~n"),
  io:format("result: ~tp~n", [apl:eval_LB("+/", apl:e2a([4,5,6]))]),
   
   % call niladic APL function 'TIME'
  apl:command("]log 32"),
  io:format("~ncall niladic APL function 'TIME'...~n"),
  io:format("result: ~tp~n", [apl:eval_("TIME")]),

  % display an Erlang term returned from an APL interface function
  %
  io:format("~ncreate 4-4 unity matrix and display it.~n"),
  [I6] = apl:statement("(⍳6)∘.=⍳6"),
  show(I6),
  ok.

% create a two-line APL function named FOO. FOO has one argument B and returns
% it, incremented by 1. Like in Erlang, the first line of the function is its
% header (defining the name and the arguments of the function), followed by
% the lines of the function body. In APL, 'fixing a function' is APL % slang
% and means defining or changing a user-defined function.
%
fix_FOO() ->
apl:fix_function_ucs([
"Z←FOO B",
"Z←B + 1"            ]).

% create a niladic APL function named TIME, which returns the current time.
%
fix_TIME() ->
apl:fix_function_ucs([
"Z←TIME",
"Z←⎕TS"              ]).

% display an Erlang term returned from an APL interface function
%
show(Val) ->
   {value, _Shape, _Ravel} = apl:eval_B("⍕", Val).  % format and show Val in APL

