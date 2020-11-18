#ifndef __Quad_MAP_DEFINED__
#define __Quad_MAP_DEFINED__

#include "QuadFunction.hh"

//-----------------------------------------------------------------------------
/// The implementation of ⎕MAP
class Quad_MAP : public QuadFunction
{
public:
   /// Constructor.
   Quad_MAP()
      : QuadFunction(TOK_Quad_MAP)
   {}

   static Quad_MAP * fun;          ///< Built-in function.
   static Quad_MAP  _fun;          ///< Built-in function.

protected:
   /// overloaded Function::eval_AB()
   virtual Token eval_AB(Value_P A, Value_P B) const;

   /// Heapsort helper
   static bool greater_map(const ShapeItem & a, const ShapeItem & b,
                           const void * cells);

   /// compute ⎕MAP with (indices of) sorted A
   static Value_P do_map(const Cell * ravel_A, ShapeItem len_A,
                  const ShapeItem * sorted_indices_A, const Value * B,
                  bool recursive);
};
//-----------------------------------------------------------------------------

#endif // __Quad_MAP_DEFINED__

