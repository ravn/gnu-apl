/* erlang_APL_nif.c */

#include <semaphore.h>
#include <stdio.h>
#include <string.h>
#include <erl_nif.h>
#include <apl/libapl.h>

#if ERL_NIF_MAJOR_VERSION == 2 && ERL_NIF_MINOR_VERSION >= 11
# define HAVE_DIRTY
#elif ERL_NIF_MAJOR_VERSION > 2
# define HAVE_DIRTY   /* we hope so */
#endif

#if HAVE_DIRTY
# define DIRTY_FLAG , ERL_NIF_DIRTY_JOB_CPU_BOUND
#else
# define DIRTY_FLAG
#endif

//=============================================================================
static int init_done = 0;

struct value_callback_context
{
  int statement_result;
  ErlNifEnv * env;
  ERL_NIF_TERM retval_idx;
};
static struct value_callback_context vc_ctx;

static sem_t if_sema;

enum
{
   UCS_maxbuf = 20000,
   UTF8_maxbuf = UCS_maxbuf * sizeof(int)
};

/// a buffer for \b UCS_maxbuf Unicodes plus a terminating 0. This buffer is
/// used by the top-level functions called from Erlang to convert Unicode and
/// UTF8 character lists into Unicode and UTF8 character vectors.
///
static unsigned int UCS_buffer[UCS_maxbuf + 1];

/// same buffer different type
#define UTF8_buffer ((char *)UCS_buffer)

/// same buffer different type
#define TERM_buffer ((ERL_NIF_TERM *)UCS_buffer)

/// current index into UCS_buffer or UTF8_buffer
static int buffer_idx = 0;

/// convert an APL value into an Erlang Term to be transmitted over the nif.
static ERL_NIF_TERM make_value(const APL_value value);

//-----------------------------------------------------------------------------

/// convert the \b idx'th cell (= ravel utem) of APL value into an Erlang term
static ERL_NIF_TERM
make_cell(const APL_value value, uint64_t idx)
{
   switch(get_type(value, idx))
      {
        case CCT_CHAR:    return enif_make_int   (vc_ctx.env,
                                                  get_char(value, idx));
        case CCT_INT:     return enif_make_int64 (vc_ctx.env,
                                                  get_int(value, idx));
        case CCT_FLOAT:   return enif_make_double(vc_ctx.env,
                                                  get_real(value, idx));
        case CCT_COMPLEX: return enif_make_tuple3(vc_ctx.env,
                                    enif_make_atom(vc_ctx.env, "complex"),
                                    enif_make_double(vc_ctx.env,
                                                     get_real(value, idx)),
                                    enif_make_double(vc_ctx.env,
                                                     get_imag(value, idx)));

        case CCT_POINTER: return make_value(get_value(value, idx));
      }

   return enif_make_atom(vc_ctx.env, "bad_cell");
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
bad_argument(const char * function, int line)
{
   fprintf(stderr, "\r\n*** Bad argument in function %s at %s:%d\r\n",
           __FILE__, function, line);
   return enif_make_badarg(vc_ctx.env);
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
make_value(const APL_value value)
{
const int64_t element_count = get_element_count(value);
ERL_NIF_TERM shape;
ERL_NIF_TERM ravel;
int64_t e;

   switch (get_rank(value))
      {
        case 0: shape = enif_make_list(vc_ctx.env, 0);
                break;

        case 1: shape = enif_make_list1(vc_ctx.env,
                            enif_make_int64(vc_ctx.env, get_axis(value, 0)));
                break;

        case 2: shape = enif_make_list2(vc_ctx.env,
                            enif_make_int64(vc_ctx.env, get_axis(value, 0)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 1)));
                break;

        case 3: shape = enif_make_list3(vc_ctx.env,
                            enif_make_int64(vc_ctx.env, get_axis(value, 0)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 1)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 2)));
                break;

        case 4: shape = enif_make_list4(vc_ctx.env,
                            enif_make_int64(vc_ctx.env, get_axis(value, 0)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 1)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 2)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 3)));
                break;

        case 5: shape = enif_make_list5(vc_ctx.env,
                            enif_make_int64(vc_ctx.env, get_axis(value, 0)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 1)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 2)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 3)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 4)));
                break;

        case 6: shape = enif_make_list6(vc_ctx.env,
                            enif_make_int64(vc_ctx.env, get_axis(value, 0)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 1)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 2)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 3)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 4)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 5)));
                break;

        case 7: shape = enif_make_list7(vc_ctx.env,
                            enif_make_int64(vc_ctx.env, get_axis(value, 0)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 1)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 2)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 3)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 4)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 5)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 6)));
                break;

        case 8: shape = enif_make_list8(vc_ctx.env,
                            enif_make_int64(vc_ctx.env, get_axis(value, 0)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 1)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 2)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 3)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 4)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 5)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 6)),
                            enif_make_int64(vc_ctx.env, get_axis(value, 7)));
                break;

        default: shape = enif_make_atom(vc_ctx.env, "TODO: rank > 8");
      }

   switch(element_count)
      {
        case 0: ravel = enif_make_list(vc_ctx.env, 0);
                break;

        case 1: ravel = enif_make_list1(vc_ctx.env,
                                        make_cell(value, 0));
                break;

        case 2: ravel = enif_make_list2(vc_ctx.env,
                                        make_cell(value, 0),
                                        make_cell(value, 1));
                break;

        case 3: ravel = enif_make_list3(vc_ctx.env,
                                        make_cell(value, 0),
                                        make_cell(value, 1),
                                        make_cell(value, 2));
                break;

        case 4: ravel = enif_make_list4(vc_ctx.env,
                                        make_cell(value, 0),
                                        make_cell(value, 1),
                                        make_cell(value, 2),
                                        make_cell(value, 3));
                break;

        case 5: ravel = enif_make_list5(vc_ctx.env,
                                        make_cell(value, 0),
                                        make_cell(value, 1),
                                        make_cell(value, 2),
                                        make_cell(value, 3),
                                        make_cell(value, 4));
                break;

        case 6: ravel = enif_make_list6(vc_ctx.env,
                                        make_cell(value, 0),
                                        make_cell(value, 1),
                                        make_cell(value, 2),
                                        make_cell(value, 3),
                                        make_cell(value, 4),
                                        make_cell(value, 5));
                break;

        case 7: ravel = enif_make_list7(vc_ctx.env,
                                        make_cell(value, 0),
                                        make_cell(value, 1),
                                        make_cell(value, 2),
                                        make_cell(value, 3),
                                        make_cell(value, 4),
                                        make_cell(value, 5),
                                        make_cell(value, 6));
                break;

        case 8: ravel = enif_make_list8(vc_ctx.env,
                                        make_cell(value, 0),
                                        make_cell(value, 1),
                                        make_cell(value, 2),
                                        make_cell(value, 3),
                                        make_cell(value, 4),
                                        make_cell(value, 5),
                                        make_cell(value, 6),
                                        make_cell(value, 7));
                break;

        case 9: ravel = enif_make_list9(vc_ctx.env,
                                        make_cell(value, 0),
                                        make_cell(value, 1),
                                        make_cell(value, 2),
                                        make_cell(value, 3),
                                        make_cell(value, 4),
                                        make_cell(value, 5),
                                        make_cell(value, 6),
                                        make_cell(value, 7),
                                        make_cell(value, 8));
                break;

        default: ravel = enif_make_list9(vc_ctx.env,
                                        make_cell(value, element_count - 9),
                                        make_cell(value, element_count - 8),
                                        make_cell(value, element_count - 7),
                                        make_cell(value, element_count - 6),
                                        make_cell(value, element_count - 5),
                                        make_cell(value, element_count - 4),
                                        make_cell(value, element_count - 3),
                                        make_cell(value, element_count - 2),
                                        make_cell(value, element_count - 1));
                 for (e = element_count - 10; e >= 0; --e)
                     ravel = enif_make_list_cell(vc_ctx.env,
                                             make_cell(value, e), ravel);
      }

   return enif_make_tuple3(vc_ctx.env,
                           enif_make_atom(vc_ctx.env, "value"),
                           shape, ravel);
}
//-----------------------------------------------------------------------------
static int
value_callback(const APL_value result, int committed)
{
   vc_ctx.statement_result = committed;   // 0 or 1

   if (committed)
      {
        TERM_buffer[vc_ctx.retval_idx++] = enif_make_atom(vc_ctx.env,
                                                          "committed_value");
        return 0;   // don't print
      }
   else
      {
        TERM_buffer[vc_ctx.retval_idx++] = make_value(result);
        return 1;   // print it
      }
}
//-----------------------------------------------------------------------------
static int
load(ErlNifEnv * env, void ** priv_data, ERL_NIF_TERM load_info)
{
   if (init_done)   return 0;   // already called

   sem_init(&if_sema, 0, 1);
   init_libapl("erl", 0);
   fprintf(stderr, "libapl initialized.\r\n");

   res_callback = value_callback;
   expand_LF_to_CRLF(1);
   init_done = 1;
   return 0;
}
//=============================================================================
static ERL_NIF_TERM
do_command_UTF8(const ERL_NIF_TERM argv[])
{
ERL_NIF_TERM ret;

   if (!enif_get_string(vc_ctx.env, argv[0], UTF8_buffer, UTF8_maxbuf,
                        ERL_NIF_LATIN1))
      return bad_argument(__FUNCTION__, __LINE__);

const char * result = apl_command(UTF8_buffer);
   ret = enif_make_string(vc_ctx.env, result, ERL_NIF_LATIN1);

   free((void *)result);   // from strndup() in apl_command()
   return ret;
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
command_UTF8(ErlNifEnv * env, int argc, const ERL_NIF_TERM argv[])
{
   sem_wait(&if_sema);
   vc_ctx.env = env;
ERL_NIF_TERM ret = do_command_UTF8(argv);
   sem_post(&if_sema);
   return ret;
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
do_command_UCS(const ERL_NIF_TERM argv[])
{
ERL_NIF_TERM list = argv[0];
ERL_NIF_TERM head;
const unsigned int * result;
const unsigned int * r;

   for (buffer_idx = 0; !enif_is_empty_list(vc_ctx.env, list); ++buffer_idx)
      {
        if (!(buffer_idx < UCS_maxbuf                 &&
              enif_is_list(vc_ctx.env, list)                     &&
              enif_get_list_cell(vc_ctx.env, list, &head, &list) &&
              enif_get_uint(vc_ctx.env, head, UCS_buffer + buffer_idx))) 
           return bad_argument(__FUNCTION__, __LINE__);
      }
   UCS_buffer[buffer_idx] = 0;

   result = apl_command_ucs(UCS_buffer);
   if (result == 0)   // empty list
      {
        return enif_make_list(vc_ctx.env, 0);
      }

   if (result[1] == 0)   // 1 element list
      {
         head = enif_make_uint(vc_ctx.env, result[0]);
         return enif_make_list(vc_ctx.env, 1, head);
      }

   r = result;
   while (r[1])   ++r;   // last non-zero integer in result

   head = enif_make_uint(vc_ctx.env, *r--);
   list = enif_make_list(vc_ctx.env, 1, head);
   for (;r >= result; --r)
      {
        const ERL_NIF_TERM h = enif_make_uint(vc_ctx.env, *r);
        list = enif_make_list(vc_ctx.env, 2, h, list);
      }

   free((void *)result);   // from malloc() in apl_command_ucs()
   return list;
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
command_UCS(ErlNifEnv * env, int argc, const ERL_NIF_TERM argv[])
{
   sem_wait(&if_sema);
   vc_ctx.env = env;
ERL_NIF_TERM ret = do_command_UCS(argv);
   sem_post(&if_sema);
   return ret;
}
//=============================================================================
static ERL_NIF_TERM
do_statement_UTF8(const ERL_NIF_TERM argv[])
{
int j;
   if (!enif_get_string(vc_ctx.env, argv[0], UTF8_buffer,
                        UTF8_maxbuf, ERL_NIF_LATIN1))
      return bad_argument(__FUNCTION__, __LINE__);

   vc_ctx.statement_result = -1;
   vc_ctx.retval_idx = 0;
const int err = apl_exec(UTF8_buffer);

   if (vc_ctx.statement_result == -1)
      {
        // if statement_result == -1 then value_callback() above was not called
        // because some APL error has occurred. In this case the error code is
        // the one returned by apl_exec(). The error code err is (to the
        // extent reasonable) the same as defined for ⎕ET on page 287 of the
        // IBM APL2 language Reference manual.
        //
        return enif_make_tuple2(vc_ctx.env,
                                enif_make_atom(vc_ctx.env, "APL_error"),
                                enif_make_int(vc_ctx.env, err));
      }

   if (vc_ctx.retval_idx == 0)   return enif_make_list(vc_ctx.env, 0);
   if (vc_ctx.retval_idx == 1)   return enif_make_list(vc_ctx.env, 1,
                                        TERM_buffer[0]);
ERL_NIF_TERM ret = enif_make_list(vc_ctx.env, 2,
                                  TERM_buffer[vc_ctx.retval_idx - 1],
                                  TERM_buffer[vc_ctx.retval_idx - 2]);
   for (j = vc_ctx.retval_idx - 3; j > 0; --j)
       ret = enif_make_list_cell(vc_ctx.env,
                                 TERM_buffer[j], ret);

   return ret;
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
statement_UTF8(ErlNifEnv * env, int argc, const ERL_NIF_TERM argv[])
{
   sem_wait(&if_sema);
   vc_ctx.env = env;
ERL_NIF_TERM ret = do_statement_UTF8(argv);
   sem_post(&if_sema);
   return ret;
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
do_statement_UCS(const ERL_NIF_TERM argv[])
{
ERL_NIF_TERM list = argv[0];
ERL_NIF_TERM head;
int j;

   for (buffer_idx = 0; !enif_is_empty_list(vc_ctx.env, list); ++buffer_idx)
      {
        if (!(buffer_idx < UCS_maxbuf                     &&
              enif_is_list(vc_ctx.env, list)                     &&
              enif_get_list_cell(vc_ctx.env, list, &head, &list) &&
              enif_get_uint(vc_ctx.env, head, UCS_buffer + buffer_idx)))
           return bad_argument(__FUNCTION__, __LINE__);
      }
   UCS_buffer[buffer_idx] = 0;

   vc_ctx.statement_result = -1;
   vc_ctx.retval_idx = 0;
const int err = apl_exec_ucs(UCS_buffer);

   if (vc_ctx.statement_result == -1)
      {
        // if statement_result == -1 then value_callback() above was not called
        // because some APL error has occurred. In this case the error code is
        // the one returned by apl_exec(). The error code err is (to the
        // extent reasonable) the same as defined for ⎕ET on page 287 of the
        // IBM APL2 language Reference manual.
        //
        return enif_make_tuple2(vc_ctx.env,
                                enif_make_atom(vc_ctx.env, "APL_error"),
                                enif_make_int(vc_ctx.env, err));
      }

   if (vc_ctx.retval_idx == 0)   return enif_make_list(vc_ctx.env, 0);
   if (vc_ctx.retval_idx == 1)   return enif_make_list(vc_ctx.env, 1,
                                        TERM_buffer[0]);
ERL_NIF_TERM ret = enif_make_list(vc_ctx.env, 2,
                                  TERM_buffer[vc_ctx.retval_idx - 1],
                                  TERM_buffer[vc_ctx.retval_idx - 2]);
   for (j = vc_ctx.retval_idx - 3; j > 0; --j)
       ret = enif_make_list_cell(vc_ctx.env,
                                 TERM_buffer[j], ret);

   return ret;
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
statement_UCS(ErlNifEnv * env, int argc, const ERL_NIF_TERM argv[])
{
   sem_wait(&if_sema);
   vc_ctx.env = env;
ERL_NIF_TERM ret = do_statement_UCS(argv);
   sem_post(&if_sema);
   return ret;
}
//=============================================================================
static ERL_NIF_TERM
do_fix_function_UCS(const ERL_NIF_TERM argv[])
{
ERL_NIF_TERM line_list = argv[0];
ERL_NIF_TERM char_list;
ERL_NIF_TERM head;
int j;

   buffer_idx = 0;
   UCS_buffer[buffer_idx++] = 0x2395;  // ⎕
   UCS_buffer[buffer_idx++] = 'F';
   UCS_buffer[buffer_idx++] = 'X';

   for (; !enif_is_empty_list(vc_ctx.env, line_list);)   // line loop
      {
        // extract one line from line_list and initialize char_list with it
        //
       if (!enif_get_list_cell(vc_ctx.env, line_list, &char_list, &line_list))
           return bad_argument(__FUNCTION__, __LINE__);

       UCS_buffer[buffer_idx++] = ' ';
       UCS_buffer[buffer_idx++] = '\'';

        for (; !enif_is_empty_list(vc_ctx.env, char_list);)   // character loop
            {
              if (!(buffer_idx < (UCS_maxbuf - 5)                         &&
                    enif_is_list(vc_ctx.env, line_list)                          &&
                    enif_get_list_cell(vc_ctx.env, char_list, &head, &char_list) &&
                    enif_get_uint(vc_ctx.env, head, UCS_buffer + buffer_idx++)))
                 return bad_argument(__FUNCTION__, __LINE__);

              if (UCS_buffer[buffer_idx] == '\'')   // duplicate single quotes
                 {
                    UCS_buffer[buffer_idx++] = '\'';
                 }
            }
        UCS_buffer[buffer_idx++] = '\'';
      }
   UCS_buffer[buffer_idx] = 0;

   vc_ctx.statement_result = -1;
   vc_ctx.retval_idx = 0;
// print_ucs(stderr, UCS_buffer);
const int err = apl_exec_ucs(UCS_buffer);

   if (vc_ctx.statement_result == -1)
      {
        // if statement_result == -1 then value_callback() above was not called
        // because some APL error has occurred. In this case the error code is
        // the one returned by apl_exec(). The error code err is (to the
        // extent reasonable) the same as defined for ⎕ET on page 287 of the
        // IBM APL2 language Reference manual.
        //
        return enif_make_tuple2(vc_ctx.env,
                                enif_make_atom(vc_ctx.env, "APL_error"),
                                enif_make_int(vc_ctx.env, err));
      }

   if (vc_ctx.retval_idx == 0)   return enif_make_list(vc_ctx.env, 0);
   if (vc_ctx.retval_idx == 1)   return enif_make_list(vc_ctx.env, 1,
                                        TERM_buffer[0]);
ERL_NIF_TERM ret = enif_make_list(vc_ctx.env, 2,
                                  TERM_buffer[vc_ctx.retval_idx - 1],
                                  TERM_buffer[vc_ctx.retval_idx - 2]);
   for (j = vc_ctx.retval_idx - 3; j > 0; --j)
       ret = enif_make_list_cell(vc_ctx.env,
                                 TERM_buffer[j], ret);

   return ret;
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
fix_function_UCS(ErlNifEnv * env, int argc, const ERL_NIF_TERM argv[])
{
   sem_wait(&if_sema);
   vc_ctx.env = env;
ERL_NIF_TERM ret = do_fix_function_UCS(argv);
   sem_post(&if_sema);
   return ret;
}
//=============================================================================
static ERL_NIF_TERM
fill_variable(const unsigned int * var_name, ERL_NIF_TERM var_shape,
              ERL_NIF_TERM var_ravel, APL_value * aval)
{
int ravel_idx = 0;
uint64_t ravel_len = 1;
ERL_NIF_TERM head;

uint64_t shape[8];
int shape_idx = 0;
   for (shape_idx = 0; !enif_is_empty_list(vc_ctx.env, var_shape); ++shape_idx)
      {
        if (!(shape_idx < 8                                                &&
              enif_is_list(vc_ctx.env, var_shape)                          &&
              enif_get_list_cell(vc_ctx.env, var_shape, &head, &var_shape) &&
              enif_get_uint64(vc_ctx.env, head, shape + shape_idx)))
           return bad_argument(__FUNCTION__, __LINE__);
        ravel_len *= shape[shape_idx];
      }
   shape[shape_idx] = 0;

   // get a variable with the given shape and its ravel set to 0, so that the
   // variable will be properly initialized even if setting of the ravel below
   // should fail.
   //
   if (ravel_len == 0)   ravel_len = 1;   // empty value: need at least 1 item
APL_value var = *aval = assign_var(var_name, shape);
   if (var == 0)     return bad_argument(__FUNCTION__, __LINE__);

   for (ravel_idx = 0; ravel_idx < ravel_len; ++ravel_idx)
      {
        // it is OK if the Erlang user provides fewer elements than needed,
        // since we have initialized the entire value to 0 above.
        //
        if (enif_is_empty_list(vc_ctx.env, var_ravel))   break;

        if (!enif_get_list_cell(vc_ctx.env, var_ravel, &head, &var_ravel))
           return bad_argument(__FUNCTION__, __LINE__);

        ErlNifSInt64 int_num = 0;
        if (enif_get_int64(vc_ctx.env, head, &int_num))   // integer value
           {
             set_int(int_num, var, ravel_idx);
             continue;
           }

        double double_real = 0;
        if (enif_get_double(vc_ctx.env, head, &double_real))
           {
             set_double(double_real, var, ravel_idx);
             continue;
           }

        // last chance: complex {real,imag} or nested value {shape,ravel}
        //
        int tuple_arity = 0;
        const ERL_NIF_TERM * tuple_items = 0;
        if (!enif_get_tuple(vc_ctx.env, head, &tuple_arity, &tuple_items))
           return bad_argument(__FUNCTION__, __LINE__);


        if (tuple_items == 0)   return bad_argument(__FUNCTION__,__LINE__);
        if (tuple_arity != 2)   return bad_argument(__FUNCTION__,__LINE__);

        ERL_NIF_TERM T0  = tuple_items[0];
        ERL_NIF_TERM T1  = tuple_items[1];
        if (enif_is_list(vc_ctx.env, T0))   // shape of a value
           {
             APL_value asub = 0;
             ERL_NIF_TERM sub = fill_variable(0, T0, T1, &asub);
             if (asub == 0)   return sub;
             if (!enif_get_int64(vc_ctx.env, sub, &int_num))
                return bad_argument(__FUNCTION__, __LINE__);
             if (int_num != 0)
                return bad_argument(__FUNCTION__, __LINE__);

             set_value(asub, var, ravel_idx);
             release_value(asub, "erlang_APL_nif.c:691");
             continue;
           }

        // complex: {real,imag}
        //
        if (enif_get_int64(vc_ctx.env, T0, &int_num))
           {
             double_real = int_num;
           }
        else if (!enif_get_double(vc_ctx.env, T0, &double_real))
           {
             return bad_argument(__FUNCTION__, __LINE__);
           }
     
        double double_imag = 0;
        if (enif_get_int64(vc_ctx.env, T1, &int_num))
           {
             double_imag = int_num;
           }
        else if (!enif_get_double(vc_ctx.env, T1, &double_imag))
           {
             return bad_argument(__FUNCTION__, __LINE__);
           }

        set_complex(double_real, double_imag, var, ravel_idx);
      }

   return enif_make_int64(vc_ctx.env, 0);
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
do_set_variable(const ERL_NIF_TERM argv[])
{
ERL_NIF_TERM var_name = argv[0];
ERL_NIF_TERM head;
APL_value dummy;

   for (buffer_idx = 0; !enif_is_empty_list(vc_ctx.env, var_name); ++buffer_idx)
      {
        if (!(buffer_idx < UCS_maxbuf                                    &&
              enif_is_list(vc_ctx.env, var_name)                         &&
              enif_get_list_cell(vc_ctx.env, var_name, &head, &var_name) &&
              enif_get_uint(vc_ctx.env, head, UCS_buffer + buffer_idx)))
           return bad_argument(__FUNCTION__, __LINE__);
      }
   UCS_buffer[buffer_idx] = 0;
   fill_variable(UCS_buffer, argv[1], argv[2], &dummy);
   return enif_make_atom(vc_ctx.env, "ok");
}
//-----------------------------------------------------------------------------
static ERL_NIF_TERM
set_variable(ErlNifEnv * env, int argc, const ERL_NIF_TERM argv[])
{
   sem_wait(&if_sema);
   vc_ctx.env = env;
ERL_NIF_TERM ret = do_set_variable(argv);
   sem_post(&if_sema);
   return ret;
}
//=============================================================================
static ErlNifFunc
nif_funcs[] = {
     // erlang name      #args   C function       flags (if supported)
   { "command_utf8",     1,      command_UTF8     DIRTY_FLAG },
   { "command_ucs",      1,      command_UCS      DIRTY_FLAG },
   { "statement_utf8",   1,      statement_UTF8   DIRTY_FLAG },
   { "statement_ucs",    1,      statement_UCS    DIRTY_FLAG },
   { "fix_function_ucs", 1,      fix_function_UCS DIRTY_FLAG },
   { "set_variable",     3,      set_variable     DIRTY_FLAG }
};
//-----------------------------------------------------------------------------

ERL_NIF_INIT(apl, nif_funcs, load, NULL, NULL, NULL)

