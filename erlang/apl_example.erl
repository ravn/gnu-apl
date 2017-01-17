%% -*- coding: utf-8 -*-

-module(apl_example).
-export([start/0]).

start() ->
  apl:init(),
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

  % create an APL function named FOO
  io:format("~nCreate (aka. fix) APL function FOO...~n"),
  io:format("~w~n", [fix_FOO()]),

  % show all APL functions
  io:format("~nShow all APL functions...~n"),
  io:format("~ts~n", [apl:command(")FNS")]),

  % display APL function FOO (a 2 line by 7 columns character matrix)
  io:format("~ndisplay APL function FOO...~n"),
  io:format("~tp~n", [apl:statement("⎕CR 'FOO'")]),

  % create a 10 by 30 integer matrix of 0s and and initialize the first
  % few elements of it from an Erlang value with integers floats, and complex
  io:format("~nSet variabe Y directly from Erlang...~n"),
  io:format("~s~n", [apl:set_variable("Y", [10,30],
                                      [1, 2.2, {3,4},{[3],"BAR"}])]),

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

  ok.

% create a two-line APL function named FOO. FOO has one argument B and returns
% it, incremented by 1. Like in Erlang, the first line of the function is its
% header (defining the name and the arguments of the function), followed by
% the lines of the function body. In APL, 'fixing a function' is APL % slang
% and means defining or changing a user-defined function.
%
fix_FOO() -> apl:fix_function_ucs([
"Z←FOO B",
"Z←B + 1"                         ]).

