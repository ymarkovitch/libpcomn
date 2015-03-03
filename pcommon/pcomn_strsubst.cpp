/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_strsubst.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2015. All rights reserved.

 DESCRIPTION  :   String substitution a la Perl templates or Python.Template class.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Dec 2007
*******************************************************************************/
#include <pcomn_strsubst.h>
#include <pcomn_calgorithm.h>
#include <pcomn_strslice.h>

namespace pcomn {
namespace tpl {

/*******************************************************************************
 substitution_map
*******************************************************************************/
substitution_map::substitution_map()
{}

substitution_map::~substitution_map()
{
   clear_icontainer(_replacement_map) ;
}

void substitution_map::replace_substfn(replacement_function *&&value)
{
   std::unique_ptr<replacement_function> guard (value) ;
   replacement_function *oldval = nullptr ;
   _replacement_map.replace(value, oldval) ;
   guard.release() ;
   delete oldval ;
}

substitution_map &substitution_map::operator()(const strslice &placeholder, const strslice &value)
{
   replace_substfn(std::move(new (placeholder, value) replacement_string(placeholder, value))) ;
   return *this ;
}

bool substitution_map::consume_char(int c, local_state &local) const
{
   switch (local.state)
   {
      case S_TEXT:
      {
         if (local.endp == local.textbuf_end || c == -1)
         {
            local.flush_text() ;
            if (c == -1)
               break ;
         }
         if (c != '$')
            *local.endp++ = c ;
         else
         {
            local.flush_text() ;
            local.placeholder.reserve(8) ;
            local.state = S_PLACEHOLDER_START ;
         }
      }
      break ;

      case S_PLACEHOLDER_START:
         switch (c)
         {
            case '$': // "$$"
               *local.endp++ = c ;
               local.state = S_TEXT ;
               break ;

            case '{':
               local.state = S_PLACEHOLDER_QUOTED ;
               break ;

            case '*':
               local.state = S_COMMENTS ;
               break ;

            default:
               if (isascii(c) && (isalpha(c) || c == '_'))
               {
                  local.placeholder.push_back(c) ;
                  local.state = S_PLACEHOLDER ;
               }
               else
               {
                  *local.endp++ = '$' ;
                  if (c == -1)
                     local.flush_text() ;
                  else
                     *local.endp++ = c ;
                  local.state = S_TEXT ;
               }
               break ;
         }
         break ;

      case S_PLACEHOLDER:
         if (isascii(c) && (isalnum(c) || c == '_'))
            local.placeholder.push_back(c) ;
         else
         {
            commit_substitution(local.out, strslice(&*local.placeholder.begin(), &*local.placeholder.end()), "$") ;
            local.finish_placeholder(c) ;
         }
         break ;

      case S_PLACEHOLDER_QUOTED:
         if (isascii(c) && (isalnum(c) || c == '_'))
            local.placeholder.push_back(c) ;
         else if (c == '}')
         {
            if (local.placeholder.empty())
               local.out("${}", 3) ;
            else
            {
               commit_substitution(local.out, strslice(&*local.placeholder.begin(), &*local.placeholder.end()), "${", "}") ;
               // We must not call finish_placeholder() since there is no next character
               // to pass into finish_placeholder() here.
               local.placeholder.clear() ;
            }
            local.state = S_TEXT ;
         }
         else
         {
            local.out("${", 2) ;
            local.out(&*local.placeholder.begin(), local.placeholder.size()) ;
            local.finish_placeholder(c) ;
         }
         break ;

      case S_COMMENTS:
         if ((c == -1) || (c == '$' && !local.placeholder.empty() &&
                           local.placeholder.back() == '*'))
         {
            local.placeholder.clear() ;
            local.state = S_TEXT ;
            break ;
         }
         local.placeholder.push_back(c) ;
         break ;
   }
   return c != -1 ;
}

void substitution_map::commit_substitution(detail::output &out, const strslice &placeholder,
                                           const char *prefix, const char *suffix) const
{
   if (const replacement_function *fn = get_keyed_value(_replacement_map, placeholder))
      fn->write(out) ;
   else if (_replacement_def)
      _replacement_def->write(placeholder, out) ;
   else {
      // No replacement for this placeholder: output the placeholder verbatim.
      if (*prefix)
         out(prefix, strlen(prefix)) ;
      out(placeholder.begin(), placeholder.size()) ;
      if (*suffix)
         out(prefix, strlen(prefix)) ;
   }
}

} // end of namespace pcomn::tpl
} // end of namespace pcomn
