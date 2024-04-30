// file      : odb/context.cxx
// license   : GNU GPL v3; see accompanying LICENSE file

#include <odb/gcc.hxx>

#include <cctype> // std::toupper
#include <cassert>
#include <sstream>

#include <odb/context.hxx>
#include <odb/common.hxx>
#include <odb/pragma.hxx>
#include <odb/cxx-lexer.hxx>
#include <odb/diagnostics.hxx>

#include <odb/relational/mssql/context.hxx>
#include <odb/relational/mysql/context.hxx>
#include <odb/relational/oracle/context.hxx>
#include <odb/relational/pgsql/context.hxx>
#include <odb/relational/sqlite/context.hxx>

using namespace std;

static inline void
add_space (string& s)
{
  string::size_type n (s.size ());
  if (n != 0 && s[n - 1] != ' ')
    s += ' ';
}

//
// custom_cxx_type
//
string custom_cxx_type::
translate (string const& val, const cxx_tokens& expr)
{
  // Similar to member_access::translate() and a few other places.
  //
  string r;

  cxx_tokens_lexer l;
  l.start (expr);

  string tl;
  for (cpp_ttype tt (l.next (tl)), ptt (CPP_EOF); tt != CPP_EOF;)
  {
    // Try to format the expression to resemble the style of the
    // generated code.
    //
    switch (tt)
    {
    case CPP_NOT:
      {
        add_space (r);
        r += '!';
        break;
      }
    case CPP_COMMA:
      {
        r += ", ";
        break;
      }
    case CPP_OPEN_PAREN:
      {
        if (ptt == CPP_NAME ||
            ptt == CPP_KEYWORD)
          add_space (r);

        r += '(';
        break;
      }
    case CPP_CLOSE_PAREN:
      {
        r += ')';
        break;
      }
    case CPP_OPEN_SQUARE:
      {
        r += '[';
        break;
      }
    case CPP_CLOSE_SQUARE:
      {
        r += ']';
        break;
      }
    case CPP_OPEN_BRACE:
      {
        add_space (r);
        r += "{ ";
        break;
      }
    case CPP_CLOSE_BRACE:
      {
        add_space (r);
        r += '}';
        break;
      }
    case CPP_SEMICOLON:
      {
        r += ';';
        break;
      }
    case CPP_ELLIPSIS:
      {
        add_space (r);
        r += "...";
        break;
      }
    case CPP_PLUS:
    case CPP_MINUS:
      {
        bool unary (ptt != CPP_NAME &&
                    ptt != CPP_SCOPE &&
                    ptt != CPP_NUMBER &&
                    ptt != CPP_STRING &&
                    ptt != CPP_CLOSE_PAREN &&
                    ptt != CPP_PLUS_PLUS &&
                    ptt != CPP_MINUS_MINUS);

        if (!unary)
          add_space (r);

        r += cxx_lexer::token_spelling[tt];

        if (!unary)
          r += ' ';
        break;
      }
    case CPP_PLUS_PLUS:
    case CPP_MINUS_MINUS:
      {
        if (ptt != CPP_NAME &&
            ptt != CPP_CLOSE_PAREN &&
            ptt != CPP_CLOSE_SQUARE)
          add_space (r);

        r += cxx_lexer::token_spelling[tt];
        break;
      }
    case CPP_DEREF:
    case CPP_DEREF_STAR:
    case CPP_DOT:
    case CPP_DOT_STAR:
      {
        r += cxx_lexer::token_spelling[tt];
        break;
      }
    case CPP_STRING:
      {
        if (ptt == CPP_NAME ||
            ptt == CPP_KEYWORD ||
            ptt == CPP_STRING ||
            ptt == CPP_NUMBER)
          add_space (r);

        r += context::strlit (tl);
        break;
      }
    case CPP_NUMBER:
      {
        if (ptt == CPP_NAME ||
            ptt == CPP_KEYWORD ||
            ptt == CPP_STRING ||
            ptt == CPP_NUMBER)
          add_space (r);

        r += tl;
        break;
      }
    case CPP_SCOPE:
      {
        // Add space except for a few common cases.
        //
        if (ptt != CPP_NAME &&
            ptt != CPP_OPEN_PAREN &&
            ptt != CPP_OPEN_SQUARE)
          add_space (r);

        r += cxx_lexer::token_spelling[tt];
        break;
      }
    case CPP_NAME:
      {
        // Start of a name.
        //
        if (ptt == CPP_NAME ||
            ptt == CPP_KEYWORD ||
            ptt == CPP_STRING ||
            ptt == CPP_NUMBER)
          add_space (r);

        r += tl;
        break;
      }
    case CPP_QUERY:
      {
        if (ptt == CPP_OPEN_PAREN)
        {
          // Get the next token and see if it is ')'.
          //
          ptt = tt;
          tt = l.next (tl);

          if (tt == CPP_CLOSE_PAREN)
            r += val;
          else
          {
            add_space (r);
            r += "? ";
          }
          continue; // We have already gotten the next token.
        }
      }
      // Fall through.
    default:
      {
        // Handle CPP_KEYWORD here to avoid a warning (it is not
        // part of the cpp_ttype enumeration).
        //
        if (tt == CPP_KEYWORD)
        {
          if (ptt == CPP_NAME ||
              ptt == CPP_KEYWORD ||
              ptt == CPP_STRING ||
              ptt == CPP_NUMBER)
            add_space (r);

          r += tl;
        }
        else
        {
          // All the other operators.
          //
          add_space (r);
          r += cxx_lexer::token_spelling[tt];
          r += ' ';
        }
        break;
      }
    }

    //
    // Watch out for the continue statements above if you add any
    // logic here.
    //

    ptt = tt;
    tt = l.next (tl);
  }

  return r;
}


//
// view_object
//

string view_object::
name () const
{
  if (!alias.empty ())
    return alias;

  return kind == object ? context::class_name (*obj) : tbl_name.string ();
}

//
// member_access
//

bool member_access::
placeholder () const
{
  for (cxx_tokens::const_iterator i (expr.begin ()), e (expr.end ()); i != e;)
  {
    if (i->type == CPP_OPEN_PAREN)
    {
      if (++i != e && i->type == CPP_QUERY)
      {
        if (++i != e && i->type == CPP_CLOSE_PAREN)
          return true;
      }
    }
    else
      ++i;
  }

  return false;
}

string member_access::
translate (string const& obj, string const& val, string const& db) const
{
  if (empty ())
  {
    error (loc) << "non-empty " << kind << " expression required" << endl;
    throw operation_failed ();
  }

  // This code is similar to translate_expression() from relations/source.cxx.
  //
  string r;

  cxx_tokens_lexer l;
  l.start (expr);

  string tl;
  for (cpp_ttype tt (l.next (tl)), ptt (CPP_EOF); tt != CPP_EOF;)
  {
    // Try to format the expression to resemble the style of the
    // generated code.
    //
    switch (tt)
    {
    case CPP_COMMA:
      {
        r += ", ";
        break;
      }
    case CPP_OPEN_PAREN:
      {
        if (ptt == CPP_NAME ||
            ptt == CPP_KEYWORD)
          add_space (r);

        r += '(';
        break;
      }
    case CPP_CLOSE_PAREN:
      {
        r += ')';
        break;
      }
    case CPP_OPEN_SQUARE:
      {
        r += '[';
        break;
      }
    case CPP_CLOSE_SQUARE:
      {
        r += ']';
        break;
      }
    case CPP_OPEN_BRACE:
      {
        add_space (r);
        r += "{ ";
        break;
      }
    case CPP_CLOSE_BRACE:
      {
        add_space (r);
        r += '}';
        break;
      }
    case CPP_SEMICOLON:
      {
        r += ';';
        break;
      }
    case CPP_ELLIPSIS:
      {
        add_space (r);
        r += "...";
        break;
      }
    case CPP_PLUS:
    case CPP_MINUS:
      {
        bool unary (ptt != CPP_NAME &&
                    ptt != CPP_SCOPE &&
                    ptt != CPP_NUMBER &&
                    ptt != CPP_STRING &&
                    ptt != CPP_CLOSE_PAREN &&
                    ptt != CPP_PLUS_PLUS &&
                    ptt != CPP_MINUS_MINUS);

        if (!unary)
          add_space (r);

        r += cxx_lexer::token_spelling[tt];

        if (!unary)
          r += ' ';
        break;
      }
    case CPP_PLUS_PLUS:
    case CPP_MINUS_MINUS:
      {
        if (ptt != CPP_NAME &&
            ptt != CPP_CLOSE_PAREN &&
            ptt != CPP_CLOSE_SQUARE)
          add_space (r);

        r += cxx_lexer::token_spelling[tt];
        break;
      }
    case CPP_DEREF:
    case CPP_DEREF_STAR:
    case CPP_DOT:
    case CPP_DOT_STAR:
      {
        r += cxx_lexer::token_spelling[tt];
        break;
      }
    case CPP_STRING:
      {
        if (ptt == CPP_NAME ||
            ptt == CPP_KEYWORD ||
            ptt == CPP_STRING ||
            ptt == CPP_NUMBER)
          add_space (r);

        r += context::strlit (tl);
        break;
      }
    case CPP_NUMBER:
      {
        if (ptt == CPP_NAME ||
            ptt == CPP_KEYWORD ||
            ptt == CPP_STRING ||
            ptt == CPP_NUMBER)
          add_space (r);

        r += tl;
        break;
      }
    case CPP_SCOPE:
      {
        // Add space except for a few common cases.
        //
        if (ptt != CPP_NAME &&
            ptt != CPP_OPEN_PAREN &&
            ptt != CPP_OPEN_SQUARE)
          add_space (r);

        r += cxx_lexer::token_spelling[tt];
        break;
      }
    case CPP_NAME:
      {
        // Start of a name.
        //
        if (ptt == CPP_NAME ||
            ptt == CPP_KEYWORD ||
            ptt == CPP_STRING ||
            ptt == CPP_NUMBER)
          add_space (r);

        r += tl;
        break;
      }
    case CPP_NOT:
    case CPP_QUERY:
      {
        if (ptt == CPP_OPEN_PAREN)
        {
          // Get the next token and see if it is ')'.
          //
          ptt = tt;
          tt = l.next (tl);

          if (tt == CPP_CLOSE_PAREN)
          {
            if (ptt == CPP_NOT)
            {
              if (db.empty ())
              {
                error (loc) << "database instance (!) not available in this "
                            << "context" << endl;
                throw operation_failed ();
              }

              r += db;
            }
            else
              r += val;
          }
          else
          {
            add_space (r);
            r += (ptt == CPP_NOT ? "!" : "? ");
          }
          continue; // We have already gotten the next token.
        }
      }
      // Fall through.
    default:
      {
        // Handle CPP_KEYWORD here to avoid a warning (it is not
        // part of the cpp_ttype enumeration).
        //
        if (tt == CPP_KEYWORD)
        {
          if (ptt == CPP_NAME ||
              ptt == CPP_KEYWORD ||
              ptt == CPP_STRING ||
              ptt == CPP_NUMBER)
            add_space (r);

          // Translate 'this'.
          //
          r += (tl == "this" ? obj : tl);
        }
        else
        {
          // All the other operators.
          //
          add_space (r);
          r += cxx_lexer::token_spelling[tt];
          r += ' ';
        }
        break;
      }
    }

    //
    // Watch out for the continue statements above if you add any
    // logic here.
    //

    ptt = tt;
    tt = l.next (tl);
  }

  return r;
}

// Sections.
//
main_section_type main_section;

bool main_section_type::
compare (object_section const& s) const
{
  main_section_type const* ms (dynamic_cast<main_section_type const*> (&s));
  return ms != 0 && *this == *ms;
}

bool user_section::
compare (object_section const& s) const
{
  user_section const* us (dynamic_cast<user_section const*> (&s));
  return us != 0 && *this == *us;
}

user_section* user_section::
total_base () const
{
  if (base != 0)
  {
    semantics::class_* poly_root (context::polymorphic (*object));
    if (poly_root != 0 && poly_root != *object)
      return base;
  }

  return 0;
}

size_t user_sections::
count (unsigned short f) const
{
  size_t r (0);

  semantics::class_* poly_root (context::polymorphic (*object));
  bool poly_derived (poly_root != 0 && poly_root != object);

  if (poly_derived && (f & count_total) != 0)
    r = context::polymorphic_base (*object).get<user_sections> (
      "user-sections").count (f);

  for (const_iterator i (begin ()); i != end (); ++i)
  {
    // Skip special sections unless we were explicitly asked to count them.
    //
    if (i->special == user_section::special_version &&
        (f & count_special_version) == 0)
      continue;

    // Skip non-versioned sections if we are only interested in the
    // versioned ones.
    //
    if ((f & count_versioned_only) != 0 &&
        !context::added (*i->member) && !context::deleted (*i->member))
      continue;

    bool ovd (i->base != 0 && poly_derived);

    if (i->load != user_section::load_eager)
    {
      if (i->load_empty ())
      {
        if ((f & count_load_empty) != 0)
        {
          if (ovd)
          {
            if ((f & count_override) != 0)
            {
              r++;
              continue; // Count each section only once.
            }
          }
          else
          {
            if ((f & count_new) != 0 || (f & count_total) != 0)
            {
              r++;
              continue;
            }
          }
        }
      }
      else
      {
        if ((f & count_load) != 0)
        {
          if (ovd)
          {
            if ((f & count_override) != 0)
            {
              r++;
              continue;
            }
          }
          else
          {
            if ((f & count_new) != 0 || (f & count_total) != 0)
            {
              r++;
              continue;
            }
          }
        }
      }
    }

    if (i->update_empty ())
    {
      if ((f & count_update_empty) != 0)
      {
        if (ovd)
        {
          if ((f & count_override) != 0)
          {
            r++;
            continue;
          }
        }
        else
        {
          if ((f & count_new) != 0 || (f & count_total) != 0)
          {
            r++;
            continue;
          }
        }
      }
    }
    else
    {
      if ((f & count_update) != 0)
      {
        if (ovd)
        {
          if ((f & count_override) != 0)
          {
            r++;
            continue;
          }
        }
        else
        {
          if ((f & count_new) != 0 || (f & count_total) != 0)
          {
            r++;
            continue;
          }
        }
      }
    }

    if (i->optimistic ())
    {
      if ((f & count_optimistic) != 0)
      {
        if (ovd)
        {
          if ((f & count_override) != 0)
          {
            r++;
            continue;
          }
        }
        else
        {
          if ((f & count_new) != 0 || (f & count_total) != 0)
          {
            r++;
            continue;
          }
        }
      }
    }
  }

  return r;
}

//
// context
//

namespace
{
  char const* keywords[] =
  {
    "NULL",
    "and",
    "asm",
    "auto",
    "bitand",
    "bitor",
    "bool",
    "break",
    "case",
    "catch",
    "char",
    "class",
    "compl",
    "const",
    "const_cast",
    "continue",
    "default",
    "delete",
    "do",
    "double",
    "dynamic_cast",
    "else",
    "end_eq",
    "enum",
    "explicit",
    "export",
    "extern",
    "false",
    "float",
    "for",
    "friend",
    "goto",
    "if",
    "inline",
    "int",
    "long",
    "mutable",
    "namespace",
    "new",
    "not",
    "not_eq",
    "operator",
    "or",
    "or_eq",
    "private",
    "protected",
    "public",
    "register",
    "reinterpret_cast",
    "return",
    "short",
    "signed",
    "sizeof",
    "static",
    "static_cast",
    "struct",
    "switch",
    "template",
    "this",
    "throw",
    "true",
    "try",
    "typedef",
    "typeid",
    "typename",
    "union",
    "unsigned",
    "using",
    "virtual",
    "void",
    "volatile",
    "wchar_t",
    "while",
    "xor",
    "xor_eq"
  };
}

unique_ptr<context>
create_context (ostream& os,
                semantics::unit& unit,
                options const& ops,
                features& f,
                semantics::relational::model* m)
{
  unique_ptr<context> r;

  switch (ops.database ()[0])
  {
  case database::common:
    {
      r.reset (new context (os, unit, ops, f));
      break;
    }
  case database::mssql:
    {
      r.reset (new relational::mssql::context (os, unit, ops, f, m));
      break;
    }
  case database::mysql:
    {
      r.reset (new relational::mysql::context (os, unit, ops, f, m));
      break;
    }
  case database::oracle:
    {
      r.reset (new relational::oracle::context (os, unit, ops, f, m));
      break;
    }
  case database::pgsql:
    {
      r.reset (new relational::pgsql::context (os, unit, ops, f, m));
      break;
    }
  case database::sqlite:
    {
      r.reset (new relational::sqlite::context (os, unit, ops, f, m));
      break;
    }
  }

  return r;
}

context::
~context ()
{
  if (current_ == this)
    current_ = 0;
}

context::
context (ostream& os_,
         semantics::unit& u,
         options_type const& ops,
         features_type& f,
         data_ptr d)
    : data_ (d ? d : data_ptr (new (shared) data (os_))),
      extra (data_->extra_),
      os (data_->os_),
      unit (u),
      options (ops),
      features (f),
      db (options.database ()[0]),
      in_comment (data_->in_comment_),
      exp (data_->exp_),
      ext (data_->ext_),
      keyword_set (data_->keyword_set_),
      include_regex (data_->include_regex_),
      accessor_regex (data_->accessor_regex_),
      modifier_regex (data_->modifier_regex_),
      embedded_schema (
        ops.generate_schema () &&
        ops.schema_format ()[db].count (schema_format::embedded)),
      separate_schema (
        ops.generate_schema () &&
        ops.schema_format ()[db].count (schema_format::separate)),
      multi_static (ops.multi_database () == multi_database::static_),
      multi_dynamic (ops.multi_database () == multi_database::dynamic),
      force_versioned (false),
      top_object (data_->top_object_),
      cur_object (data_->cur_object_)
{
  assert (current_ == 0);
  current_ = this;

  // Write boolean values as true/false.
  //
  os.setf (ios_base::boolalpha);

  // Export control.
  //
  if (!ops.export_symbol ()[db].empty ())
    exp = ops.export_symbol ()[db] + " ";

  ext = ops.extern_symbol ()[db];

  for (size_t i (0); i < sizeof (keywords) / sizeof (char*); ++i)
    data_->keyword_set_.insert (keywords[i]);

  // SQL name regex.
  //
  if (ops.table_regex ().count (db) != 0)
  {
    strings const& s (ops.table_regex ()[db]);
    data_->sql_name_regex_[sql_name_table].assign (s.begin (), s.end ());
  }

  if (ops.column_regex ().count (db) != 0)
  {
    strings const& s (ops.column_regex ()[db]);
    data_->sql_name_regex_[sql_name_column].assign (s.begin (), s.end ());
  }

  if (ops.index_regex ().count (db) != 0)
  {
    strings const& s (ops.index_regex ()[db]);
    data_->sql_name_regex_[sql_name_index].assign (s.begin (), s.end ());
  }

  if (ops.fkey_regex ().count (db) != 0)
  {
    strings const& s (ops.fkey_regex ()[db]);
    data_->sql_name_regex_[sql_name_fkey].assign (s.begin (), s.end ());
  }

  if (ops.sequence_regex ().count (db) != 0)
  {
    strings const& s (ops.sequence_regex ()[db]);
    data_->sql_name_regex_[sql_name_sequence].assign (s.begin (), s.end ());
  }

  if (ops.statement_regex ().count (db) != 0)
  {
    strings const& s (ops.statement_regex ()[db]);
    data_->sql_name_regex_[sql_name_statement].assign (s.begin (), s.end ());
  }

  if (ops.sql_name_regex ().count (db) != 0)
  {
    strings const& s (ops.sql_name_regex ()[db]);
    data_->sql_name_regex_[sql_name_all].assign (s.begin (), s.end ());
  }

  // Include regex.
  //
  for (strings::const_iterator i (ops.include_regex ().begin ());
       i != ops.include_regex ().end (); ++i)
    data_->include_regex_.push_back (regexsub (*i));

  // Common accessor/modifier naming variants. Try the user-supplied and
  // more specific ones first.
  //
  for (strings::const_iterator i (ops.accessor_regex ().begin ());
       i != ops.accessor_regex ().end (); ++i)
    data_->accessor_regex_.push_back (regexsub (*i));

  data_->accessor_regex_.push_back (regexsub ("/(.+)/get_$1/"));   // get_foo
  data_->accessor_regex_.push_back (regexsub ("/(.+)/get\\u$1/")); // getFoo
  data_->accessor_regex_.push_back (regexsub ("/(.+)/get$1/"));    // getfoo
  data_->accessor_regex_.push_back (regexsub ("/(.+)/$1/"));       // foo

  for (strings::const_iterator i (ops.modifier_regex ().begin ());
       i != ops.modifier_regex ().end (); ++i)
    data_->modifier_regex_.push_back (regexsub (*i));

  data_->modifier_regex_.push_back (regexsub ("/(.+)/set_$1/"));   // set_foo
  data_->modifier_regex_.push_back (regexsub ("/(.+)/set\\u$1/")); // setFoo
  data_->modifier_regex_.push_back (regexsub ("/(.+)/set$1/"));    // setfoo
  data_->modifier_regex_.push_back (regexsub ("/(.+)/$1/"));       // foo
}

context::
context ()
  : data_ (current ().data_),
    extra (current ().extra),
    os (current ().os),
    unit (current ().unit),
    options (current ().options),
    features (current ().features),
    db (current ().db),
    in_comment (current ().in_comment),
    exp (current ().exp),
    ext (current ().ext),
    keyword_set (current ().keyword_set),
    include_regex (current ().include_regex),
    accessor_regex (current ().accessor_regex),
    modifier_regex (current ().modifier_regex),
    embedded_schema (current ().embedded_schema),
    separate_schema (current ().separate_schema),
    multi_static (current ().multi_static),
    multi_dynamic (current ().multi_dynamic),
    force_versioned (current ().force_versioned),
    top_object (current ().top_object),
    cur_object (current ().cur_object)
{
}

context* context::current_;

semantics::data_member* context::
id (data_member_path const& mp)
{
  semantics::data_member* idf (mp.front ());

  if (!id (*idf))
    return 0;

  // This is for special ids, such as polymorphic-ref, which
  // don't have "id-member" set (and we want to keep it that
  // way since it is not really a full-fledged id).
  //
  if (idf->get<string> ("id").empty ()) // Not a nested id.
    return idf;

  const data_member_path& id (
    *id_member (
      dynamic_cast<semantics::class_&> (idf->scope ())));

  // Now we need to make sure id is a prefix of mp;
  //
  return mp.sub (id) ? idf : 0;
}

semantics::data_member* context::
object_pointer (data_member_path const& mp)
{
  for (data_member_path::const_reverse_iterator i (mp.rbegin ());
       i != mp.rend (); ++i)
  {
    if (object_pointer (utype (**i)))
      return *i;
  }

  return 0;
}

bool context::
readonly (data_member_path const& mp, data_member_scope const& ms)
{
  assert (mp.size () == ms.size ());

  data_member_scope::const_reverse_iterator si (ms.rbegin ());

  for (data_member_path::const_reverse_iterator pi (mp.rbegin ());
       pi != mp.rend ();
       ++pi, ++si)
  {
    semantics::data_member& m (**pi);

    if (m.count ("readonly"))
      return true;

    // Check if any of the classes in the inheritance chain for the
    // class containing this member are readonly.
    //
    class_inheritance_chain const& ic (*si);

    assert (ic.back () == &m.scope ());

    for (class_inheritance_chain::const_reverse_iterator ci (ic.rbegin ());
         ci != ic.rend ();
         ++ci)
    {
      semantics::class_& c (**ci);

      if (c.count ("readonly"))
        return true;
    }
  }

  return false;
}

bool context::
readonly (semantics::data_member& m)
{
  if (m.count ("readonly"))
    return true;

  // Check if the whole class (object or composite value) is marked
  // as readonly.
  //
  if (m.scope ().count ("readonly"))
    return true;

  return false;
}

bool context::
null (data_member_path const& mp) const
{
  // Outer members can override the null-ability of the inner ones. So
  // start from the most outer member.
  //
  for (data_member_path::const_iterator i (mp.begin ()); i != mp.end (); ++i)
  {
    if (null (**i))
      return true;
  }

  return false;
}

bool context::
null (semantics::data_member& m) const
{
  semantics::names* hint;
  semantics::type& t (utype (m, hint));

  if (object_pointer (t))
  {
    // By default pointers can be null.
    //
    if (m.count ("null"))
      return true;

    if (!m.count ("not-null"))
    {
      if (t.count ("null"))
        return true;

      if (!t.count ("not-null"))
        return true;
    }

    return false;
  }
  else
  {
    // Everything else by default is not null.
    //
    if (m.count ("null"))
      return true;

    if (!m.count ("not-null"))
    {
      if (t.count ("null"))
        return true;

      if (!t.count ("not-null"))
      {
        semantics::type* pt;

        // Check if this type is a wrapper.
        //
        if (t.get<bool> ("wrapper"))
        {
          // First see if it is null by default.
          //
          if (t.get<bool> ("wrapper-null-handler") &&
              t.get<bool> ("wrapper-null-default"))
            return true;

          // Otherwise, check the wrapped type.
          //
          pt = t.get<semantics::type*> ("wrapper-type");
          hint = t.get<semantics::names*> ("wrapper-hint");
          pt = &utype (*pt, hint);

          if (pt->count ("null"))
            return true;

          if (pt->count ("not-null"))
            return false;
        }
        else
          pt = &t;
      }
    }

    return false;
  }
}

bool context::
null (semantics::data_member& m, string const& kp) const
{
  if (kp.empty ())
    return null (m);

  semantics::type& c (utype (m));
  semantics::type& t (utype (m, kp));

  if (object_pointer (t))
  {
    if (m.count (kp + "-null"))
      return true;

    if (!m.count (kp + "-not-null"))
    {
      if (c.count (kp + "-null"))
        return true;

      if (!c.count (kp + "-not-null"))
      {
        if (t.count ("null"))
          return true;

        if (!t.count ("not-null"))
        {
          return true;
        }
      }
    }

    return false;
  }
  else
  {
    if (m.count (kp + "-null"))
      return true;

    if (!m.count (kp + "-not-null"))
    {
      if (c.count (kp + "-null"))
        return true;

      if (!c.count (kp + "-not-null"))
      {
        if (t.count ("null"))
          return true;

        if (!t.count ("not-null"))
        {
          semantics::type* pt;

          // Check if this type is a wrapper.
          //
          if (t.get<bool> ("wrapper"))
          {
            // First see if it is null by default.
            //
            if (t.get<bool> ("wrapper-null-handler") &&
                t.get<bool> ("wrapper-null-default"))
              return true;

            // Otherwise, check the wrapped type.
            //
            pt = t.get<semantics::type*> ("wrapper-type");
            pt = &utype (*pt);

            if (pt->count ("null"))
              return true;

            if (pt->count ("not-null"))
              return false;
          }
          else
            pt = &t;
        }
      }
    }

    return false;
  }
}

size_t context::
polymorphic_depth (semantics::class_& c)
{
  if (c.count ("polymorphic-depth"))
    return c.get<size_t> ("polymorphic-depth");

  // Calculate our hierarchy depth (number of classes).
  //
  using semantics::class_;

  class_* root (polymorphic (c));
  assert (root != 0);

  size_t r (1); // One for the root.

  for (class_* b (&c); b != root; b = &polymorphic_base (*b))
    ++r;

  c.set ("polymorphic-depth", r);
  return r;
}

context::class_kind_type context::
class_kind (semantics::class_& c)
{
  if (object (c))
    return class_object;
  else if (view (c))
    return class_view;
  else if (composite (c))
    return class_composite;
  else
    return class_other;
}

string context::
class_name (semantics::class_& c)
{
  return c.is_a<semantics::class_instantiation> ()
    ? c.get<semantics::names*> ("tree-hint")->name ()
    : c.name ();
}

string context::
class_fq_name (semantics::class_& c)
{
  return c.is_a<semantics::class_instantiation> ()
    ? c.fq_name (c.get<semantics::names*> ("tree-hint"))
    : c.fq_name ();
}

semantics::scope& context::
class_scope (semantics::class_& c)
{
  return c.is_a<semantics::class_instantiation> ()
    ? c.get<semantics::names*> ("tree-hint")->scope ()
    : c.scope ();
}

semantics::path context::
class_file (semantics::class_& c)
{
  // If we have an explicit definition location, use that.
  //
  if (c.count ("definition"))
    return semantics::path (LOCATION_FILE (c.get<location_t> ("definition")));
  //
  // Otherwise, if it is a template instantiation, use the location
  // of the qualifier.
  //
  else if (c.is_a<semantics::class_instantiation> ())
    return semantics::path (LOCATION_FILE (c.get<location_t> ("location")));
  else
    return c.file ();
}

location_t context::
class_location (semantics::class_& c)
{
  return c.count ("definition")
    ? c.get<location_t> ("definition")
    : class_real_location (c);
}

location_t context::
class_real_location (semantics::class_& c)
{
  return c.is_a<semantics::class_instantiation> ()
    ? c.get<location_t> ("location")
    : real_source_location (TYPE_NAME (c.tree_node ()));
}

string context::
upcase (string const& s)
{
  string r;
  string::size_type n (s.size ());

  r.reserve (n);

  for (string::size_type i (0); i < n; ++i)
    r.push_back (toupper (s[i]));

  return r;
}

void context::
diverge (streambuf* sb)
{
  data_->os_stack_.push (data_->os_.rdbuf ());
  data_->os_.rdbuf (sb);
}

void context::
restore ()
{
  data_->os_.rdbuf (data_->os_stack_.top ());
  data_->os_stack_.pop ();
}

semantics::type& context::
utype (semantics::type& t)
{
  if (semantics::qualifier* q = dynamic_cast<semantics::qualifier*> (&t))
    return q->base_type ();
  else
    return t;
}

semantics::type& context::
utype (semantics::type& t, semantics::names*& hint)
{
  if (semantics::qualifier* q = dynamic_cast<semantics::qualifier*> (&t))
  {
    hint = q->qualifies ().hint ();
    return q->base_type ();
  }
  else
    return t;
}

semantics::type& context::
utype (semantics::data_member& m,
       semantics::names*& hint,
       string const& kp,
       const custom_cxx_type** translation)
{
  semantics::type* t (0);

  if (kp.empty ())
  {
    t = &m.type ();

    if (semantics::qualifier* q = dynamic_cast<semantics::qualifier*> (t))
    {
      hint = q->qualifies ().hint ();
      t = &q->base_type ();
    }
    else
      hint = m.belongs ().hint ();
  }
  else
  {
    if (m.count (kp + "-tree-type"))
      t = indirect_type (m, kp, hint);
    else
    {
      t = &utype (m);

      // "See through" wrappers.
      //
      if (semantics::type* wt = wrapper (*t))
        t = indirect_type (utype (*wt), kp, hint);
      else
        t = indirect_type (*t, kp, hint);
    }
  }

  // Do we need to map this type?
  //
  // @@ Need to cache the result on the member.
  //
  if (translation != 0)
    *translation = 0;

  for (semantics::scope* s (&m.scope ());; s = &s->scope_ ())
  {
    using semantics::namespace_;

    if (namespace_* ns = dynamic_cast<namespace_*> (s))
    {
      if (ns->extension ())
        s = &ns->original ();
    }

    if (s->count ("custom-cxx-type-map"))
    {
      typedef custom_cxx_type_map map;

      map& m (s->get<map> ("custom-cxx-type-map"));
      map::const_iterator i (m.find (t));

      if (i != m.end ())
      {
        hint = i->second->as_hint;
        t = i->second->as;

        if (translation != 0)
          *translation = i->second;

        // Currently we only support one level of mapping, but I am
        // sure someone will want multiple levels.
        //
        break;
      }
    }

    if (!s->named_p () || s->global_scope ())
      break;
  }

  return *t;
}

bool context::
const_type (semantics::type& t)
{
  if (semantics::qualifier* q = dynamic_cast<semantics::qualifier*> (&t))
    return q->const_ ();

  return false;
}

string context::
type_ref_type (semantics::type& t,
               semantics::names* hint,
               bool mc,
               string const& var,
               bool decay)
{
  using semantics::array;
  string r;

  // Note that trailing const syntax is used for a reason (consider
  // t == const foo*). We may also have to decay then top-level array.
  //
  array* a;
  if (decay && (a = dynamic_cast<array*> (&utype (t))) != 0)
  {
    semantics::type& bt (a->base_type ());
    hint = a->contains ().hint ();

    if (bt.is_a<array> ())
    {
      // If we need to add/strip const or no name was used in the
      // declaration, then create an array declaration (e.g., for
      // char x[2][3] we will have char const (*x)[3]).
      //
      if (mc != const_type (t) || hint == 0)
        return type_val_type (bt, 0, mc, "(*" + var + ")");
    }

    // Array base type is always cvr-unqualified.
    //
    if (mc)
      r = bt.fq_name (hint) + " const";
    else
      r = bt.fq_name (hint);

    r += '*';

    if (!var.empty ())
      r += ' ' + var;
  }
  else
  {
    if (mc == const_type (t))
      r = t.fq_name (hint);
    else if (mc)
      r = t.fq_name (hint) + " const";
    else
    {
      semantics::type& ut (utype (t, hint));
      r = ut.fq_name (hint);
    }

    r += '&';

    if (!var.empty ())
      r += ' ' + var;
  }

  return r;
}

string context::
type_val_type (semantics::type& t,
               semantics::names* hint,
               bool mc,
               string const& var)
{
  using semantics::array;
  string r;

  // Arrays are a complicated case. Firstly, we may need to add/strip const
  // to/from the base type. Secondly, the array dimensions are written after
  // the variable name. All this is further complicated by multiple dimensions.
  // Thanks, Dennis!
  //
  if (array* a = dynamic_cast<array*> (&utype (t)))
  {
    semantics::type& bt (a->base_type ());

    // If we don't need to add/strip const and a name was used in the
    // declaration, then use that name.
    //
    if (mc == const_type (t) && hint != 0)
    {
      r = t.fq_name (hint);

      if (!var.empty ())
        r += ' ' + var;
    }
    else
    {
      // Otherwise, construct the array declaration.
      //
      string v (var);
      v += '[';
      ostringstream ostr;
      ostr << a->size ();
      v += ostr.str ();

      if (a->size () > 0xFFFFFFFF)
        v += "ULL";
      else if (a->size () > 2147483647)
        v += "U";

      v += ']';

      r = type_val_type (bt, a->contains ().hint (), mc, v);
    }
  }
  else
  {
    if (mc == const_type (t))
      r = t.fq_name (hint);
    else if (mc)
      r = t.fq_name (hint) + " const";
    else
    {
      semantics::type& ut (utype (t, hint));
      r = ut.fq_name (hint);
    }

    if (!var.empty ())
      r += ' ' + var;
  }

  return r;
}

void context::
set_member (semantics::data_member& m,
            const string& obj,
            const string& val,
            const string& db,
            const string& type)
{
  member_access& ma (m.get<member_access> ("set"));

  // If this is a custom expression, output the location of where
  // it came from.
  //
  if (!ma.synthesized)
    os << "// From " << location_string (ma.loc, true) << endl;

  if (ma.placeholder ())
  {
    // Cast the database to the concrete type this code is generated
    // for. This way the user is free to use either the concrete or
    // the common.
    //
    string d;
    if (!db.empty ())
      d = "static_cast<" + context::db.string () + "::database&> (" + db + ")";

    os << ma.translate (obj, val, d) << ";";
  }
  else
  {
    // If this member is const and we have a synthesized direct access,
    // then cast away constness. Otherwise, we assume that the user-
    // provided expression handles this.
    //
    bool cast (!type.empty () && ma.direct () && const_member (m));
    if (cast)
      os << "const_cast< " << type << "& > (" << endl;

    os << ma.translate (obj);

    if (cast)
      os << ")";

    os << " = " << val << ";";
  }
}

void context::
inc_member (semantics::data_member& m,
            const string& obj,
            const string& gobj,
            const string& type)
{
  member_access& ma (m.get<member_access> ("set"));

  // If this is a custom expression, output the location of where
  // it came from.
  //
  if (!ma.synthesized)
    os << "// From " << location_string (ma.loc, true) << endl;

  if (ma.placeholder ())
  {
    member_access& gma (m.get<member_access> ("get"));

    if (!gma.synthesized)
      os << "// From " << location_string (gma.loc, true) << endl;

    os << ma.translate (obj, gma.translate (gobj) + " + 1") << ";";
  }
  else
  {
    // If this member is const and we have a synthesized direct access,
    // then cast away constness. Otherwise, we assume that the user-
    // provided expression handles this.
    //
    os << "++";

    bool cast (ma.direct () && const_member (m));
    if (cast)
      os << "const_cast< " << type << "& > (" << endl;

    os << ma.translate (obj);

    if (cast)
      os << ")";

    os << ";";
  }
}

void context::
resolve_data_members (data_member_path& r,
                      semantics::class_& c,
                      const string& name,
                      const location& l,
                      cxx_string_lexer& lex)
{
  using semantics::class_;
  using semantics::data_member;

  // The name was already verified to be syntactically correct so
  // we don't need to do any extra error checking in this area.
  //
  lex.start (name);

  try
  {
    string tl;
    cpp_ttype tt (lex.next (tl));

    data_member& m (c.lookup<data_member> (tl, class_::include_hidden));

    r.push_back (&m);

    if (container (m))
      return;

    // Resolve nested members if any.
    //
    for (tt = lex.next (tl); tt == CPP_DOT; tt = lex.next (tl))
    {
      lex.next (tl); // Get CPP_NAME.

      data_member& om (*r.back ());

      // Check that the outer member is composite and also unwrap it while
      // at it.
      //
      class_* comp (composite_wrapper (utype (om)));
      if (comp == 0)
      {
        error (l) << "data member '" << om.name () << "' is not composite"
                  << endl;
        throw operation_failed ();
      }

      data_member& nm (
        comp->lookup<data_member> (tl, class_::include_hidden));

      r.push_back (&nm);

      if (container (nm))
        return;
    }
  }
  catch (semantics::unresolved const& e)
  {
    if (e.type_mismatch)
      error (l) << "name '" << e.name << "' does not refer to a data member"
                << endl;
    else
      error (l) << "unable to resolve data member '" << e.name << endl;

    throw operation_failed ();
  }
  catch (semantics::ambiguous const& e)
  {
    error (l) << "data member name '" << e.first.name () << "' is ambiguous"
              << endl;

    info (e.first.named ().location ()) << "could resolve to " <<
      "this data member" << endl;

    info (e.second.named ().location ()) << "or could resolve " <<
      "to this data member" << endl;

    throw operation_failed ();
  }
}

bool context::
composite_ (semantics::class_& c)
{
  bool r (c.count ("value") && !c.count ("simple") && !c.count ("container"));
  c.set ("composite-value", r);
  return r;
}

// context::table_prefix
//
context::table_prefix::
table_prefix (semantics::class_& c)
    : level (1)
{
  context& ctx (context::current ());

  ns_schema = ctx.schema (class_scope (c));
  ns_prefix = ctx.table_name_prefix (class_scope (c));
  prefix = ctx.table_name (c, &derived);
  prefix += "_";
}

void context::table_prefix::
append (semantics::data_member& m)
{
  assert (level > 0);

  context& ctx (context::current ());

  // If a custom table prefix was specified, then ignore the top-level
  // table prefix (this corresponds to a container directly inside an
  // object) but keep the schema unless the alternative schema is fully
  // qualified.
  //
  if (m.count ("table"))
  {
    qname p, n (m.get<qname> ("table"));

    if (n.fully_qualified ())
      p = n.qualifier ();
    else
    {
      if (n.qualified ())
      {
        p = ns_schema;
        p.append (n.qualifier ());
      }
      else
        p = prefix.qualifier ();
    }

    if (level == 1)
    {
      p.append (ns_prefix);
      derived = false;
    }
    else
      p.append (prefix.uname ());

    p += n.uname ();
    prefix.swap (p);
  }
  // Otherwise use the member name and add an underscore unless it is
  // already there.
  //
  else
  {
    string name (ctx.public_name_db (m));
    size_t n (name.size ());

    prefix += name;

    if (n != 0 && name[n - 1] != '_')
      prefix += "_";

    derived = true;
  }

  level++;
}

qname context::
schema (semantics::scope& s) const
{
  if (s.count ("qualified-schema"))
    return s.get<qname> ("qualified-schema");

  qname r;

  for (semantics::scope* ps (&s);; ps = &ps->scope_ ())
  {
    using semantics::namespace_;

    namespace_* ns (dynamic_cast<namespace_*> (ps));

    if (ns == 0) // Some other scope.
    {
      if (!ps->named_p ())
        break;

      continue;
    }

    if (ns->extension ())
      ns = &ns->original ();

    bool sf (ns->count ("schema"));
    bool tf (ns->count ("table"));

    if (tf)
    {
      qname n (ns->get<qname> ("table"));
      tf = n.qualified ();

      // If we have both schema and qualified table prefix, see which
      // takes precedence based on order.
      //

      if (tf && sf)
      {
        if (ns->get<location_t> ("table-location") >
            ns->get<location_t> ("schema-location"))
          sf = false;
        else
          tf = false;
      }
    }

    if (sf || tf)
    {
      qname n (
        sf
        ? ns->get<qname> ("schema")
        : ns->get<qname> ("table").qualifier ());
      n.append (r);
      n.swap (r);
    }

    if (r.fully_qualified () ||
        ns->global_scope ()) // Note: namespaces always named.
      break;
  }

  // If we are still not fully qualified, add the schema that was
  // specified on the command line.
  //
  if (!r.fully_qualified () && options.schema ().count (db) != 0)
  {
    qname n (options.schema ()[db]);
    n.append (r);
    n.swap (r);
  }

  s.set ("qualified-schema", r);
  return r;
}

string context::
table_name_prefix (semantics::scope& s) const
{
  if (s.count ("table-prefix"))
    return s.get<string> ("table-prefix");

  string r;

  for (semantics::scope* ps (&s);; ps = &ps->scope_ ())
  {
    using semantics::namespace_;

    namespace_* ns (dynamic_cast<namespace_*> (ps));

    if (ns == 0) // Some other scope.
    {
      if (!ps->named_p ())
        break;

      continue;
    }

    if (ns->extension ())
      ns = &ns->original ();

    if (ns->count ("table"))
    {
      qname n (ns->get<qname> ("table"));
      r = n.uname () + r;
    }

    if (ns->global_scope ()) // Note: namespaces always named.
      break;
  }

  // Add the prefix that was specified on the command line.
  //
  if (options.table_prefix ().count (db) != 0)
    r = options.table_prefix ()[db] + r;

  s.set ("table-prefix", r);
  return r;
}

qname context::
table_name (semantics::class_& c, bool* pd) const
{
  if (c.count ("qualified-table"))
    return c.get<qname> ("qualified-table");

  qname r;
  bool sf (c.count ("schema"));
  bool derived;

  if (c.count ("table"))
  {
    r = c.get<qname> ("table");

    if (sf)
    {
      // If we have both schema and qualified table, see which takes
      // precedence based on order. If the table is unqualifed, then
      // add the schema.
      //
      sf = !r.qualified () ||
        c.get<location_t> ("table-location") <
        c.get<location_t> ("schema-location");
    }

    derived = false;
  }
  else
  {
    r = class_name (c);
    derived = true;
  }

  if (sf)
  {
    qname n (c.get<qname> ("schema"));
    n.append (r.uname ());
    n.swap (r);
  }

  // Unless we are fully qualified, add any schemas that were
  // specified on the namespaces and/or with the command line
  // option.
  //
  if (!r.fully_qualified ())
  {
    qname n (schema (class_scope (c)));
    n.append (r);
    n.swap (r);
  }

  // Add the table prefix if any.
  //
  r.uname () = table_name_prefix (class_scope (c)) + r.uname ();

  if (derived)
    r.uname () = transform_name (r.uname (), sql_name_table);

  c.set ("qualified-table", r);

  if (pd != 0)
    *pd = derived;

  return r;
}

qname context::
table_name (semantics::class_& obj, data_member_path const& mp) const
{
  table_prefix tp (obj);

  if (mp.size () == 1)
  {
    // Container directly in the object.
    //
    return table_name (*mp.back (), tp);
  }
  else
  {
    data_member_path::const_iterator i (mp.begin ());

    // The last member is the container.
    //
    for (data_member_path::const_iterator e (mp.end () - 1); i != e; ++i)
      tp.append (**i);

    return table_name (**i, tp);
  }
}

// The table prefix passed as the second argument must include the table
// prefix specified on namespaces and with the --table-prefix option.
//
qname context::
table_name (semantics::data_member& m, table_prefix const& p) const
{
  assert (p.level > 0);
  qname r;
  string rn;
  bool derived; // Any of the components in the table name is derived.

  // If a custom table name was specified, then ignore the top-level
  // table prefix (this corresponds to a container directly inside an
  // object). If the container table is unqualifed, then we use the
  // object schema. If it is fully qualified, then we use that name.
  // Finally, if it is qualified but not fully qualifed, then we
  // append the object's namespace schema.
  //
  if (m.count ("table"))
  {
    qname n (m.get<qname> ("table"));

    if (n.fully_qualified ())
      r = n.qualifier ();
    else
    {
      if (n.qualified ())
      {
        r = p.ns_schema;
        r.append (n.qualifier ());
      }
      else
        r = p.prefix.qualifier ();
    }

    if (p.level == 1)
    {
      rn = p.ns_prefix;
      derived = false;
    }
    else
    {
      rn = p.prefix.uname ();
      derived = p.derived;
    }

    rn += n.uname ();
  }
  else
  {
    r = p.prefix.qualifier ();
    rn = p.prefix.uname () + public_name_db (m);
    derived = true;
  }

  if (derived)
    r.append (transform_name (rn, sql_name_table));
  else
    r.append (rn);

  return r;
}

string context::
table_options (semantics::class_& c)
{
  string r;

  // Accumulate options from class.
  //
  // @@ Should we also get them from bases?
  //
  // @@ Note for some databases (like SQLite), options are seperated with
  //    comma, not space. Likely the same issue in the column_options().
  //
  if (c.count ("options"))
  {
    strings const& o (c.get<strings> ("options"));

    for (strings::const_iterator i (o.begin ()); i != o.end (); ++i)
    {
      if (i->empty ())
        r.clear ();
      else
      {
        if (!r.empty ())
          r += ' ';

        r += *i;
      }
    }
  }

  return r;
}

string context::
table_options (semantics::data_member& m, semantics::type& c)
{
  string r;

  // Accumulate options from container and member.
  //
  // @@ Currently there is no way to assign options to the container type. If
  //    we use the value specifier, then it ends up being treated as a value
  //    type. To support this we will probably need to invent the container
  //    specifier.
  //
  if (c.count ("options"))
  {
    strings const& o (c.get<strings> ("options"));

    for (strings::const_iterator i (o.begin ()); i != o.end (); ++i)
    {
      if (i->empty ())
        r.clear ();
      else
      {
        if (!r.empty ())
          r += ' ';

        r += *i;
      }
    }
  }

  if (m.count ("options"))
  {
    strings const& o (m.get<strings> ("options"));

    for (strings::const_iterator i (o.begin ()); i != o.end (); ++i)
    {
      if (i->empty ())
        r.clear ();
      else
      {
        if (!r.empty ())
          r += ' ';

        r += *i;
      }
    }
  }

  return r;
}

// context::column_prefix
//
context::column_prefix::
column_prefix (data_member_path const& mp, bool l)
    : derived (false), underscore (false)
{
  if (mp.size () < (l ? 1 : 2))
    return;

  for (data_member_path::const_iterator i (mp.begin ()),
         e (mp.end () - (l ? 0 : 1)); i != e; ++i)
    append (**i);
}

void context::column_prefix::
append (semantics::data_member& m, string const& kp, string const& dn)
{
  bool d;
  context& ctx (context::current ());

  if (kp.empty ())
    prefix += ctx.column_name (m, d);
  else
    prefix += ctx.column_name (m, kp, dn, d);

  // If the user provided the column prefix, then use it verbatime.
  // Otherwise, append the underscore, unless it is already there.
  //
  underscore = false;
  if (d)
  {
    size_t n (prefix.size ());

    if (n != 0 && prefix[n - 1] != '_')
    {
      prefix += '_';
      underscore = true;
    }
  }

  derived = derived || d;
}

string context::
column_name (semantics::data_member& m, bool& derived) const
{
  derived = !m.count ("column");
  return derived
    ? public_name_db (m)
    : m.get<table_column> ("column").column;
}

string context::
column_name (semantics::data_member& m, column_prefix const& cp) const
{
  bool d;
  const string& cn (column_name (m, d));
  string n (cp.prefix);

  if (cn.empty () && cp.underscore)
    n.resize (n.size () - 1); // Strip underscore that was auto added.

  n += cn;

  // If any component is derived, then run it through the SQL name regex.
  //
  if (d || cp.derived)
    n = transform_name (n, sql_name_column);

  return n;
}

string context::
column_name (semantics::data_member& m,
             string const& p,
             string const& d,
             bool& derived) const
{
  if (p.empty () && d.empty ())
    return column_name (m, derived);

  // A container column name can be specified for the member or for the
  // container type.
  //
  string key (p + "-column");
  derived = false;

  if (m.count (key))
    return m.get<string> (key);
  else
  {
    semantics::type& t (utype (m));

    if (t.count (key))
      return t.get<string> (key);
  }

  derived = true;
  return d;
}

string context::
column_name (semantics::data_member& m,
             string const& kp,
             string const& dn,
             column_prefix const& cp) const
{
  bool d;
  const string& cn (column_name (m, kp, dn, d));
  string n (cp.prefix);

  if (cn.empty () && cp.underscore)
    n.resize (n.size () - 1); // Strip underscore that was auto-added.

  n += cn;

  // If any component is derived, the run it through the SQL name regex.
  //
  if (d || cp.derived)
    n = transform_name (n, sql_name_column);

  return n;
}

string context::
column_name (data_member_path const& mp) const
{
  return column_name (*mp.back (), column_prefix (mp));
}

string context::
column_type (const data_member_path& mp, string const& kp, bool id)
{
  if (kp.empty ())
  {
    // Return the id type if this member is or is a part of an object id
    // or pointer to object.
    //
    return mp.back ()->get<string> (
      id || context::id (mp) || object_pointer (mp)
      ? "column-id-type"
      : "column-type");
  }
  else
    return indirect_value<string> (*mp.back (), kp + "-column-type");
}

string context::
column_type (semantics::data_member& m, string const& kp)
{
  return kp.empty ()
    ? m.get<string> ("column-type")
    : indirect_value<string> (m, kp + "-column-type");
}

string context::
column_options (semantics::data_member& m)
{
  // Accumulate options from both type and member.
  //
  semantics::type* t (&utype (m));

  if (semantics::class_* p = object_pointer (*t))
    t = &utype (*id_member (*p));

  if (semantics::type* w = wrapper (*t))
    t = w;

  string r;

  if (t->count ("options"))
  {
    strings const& o (t->get<strings> ("options"));

    for (strings::const_iterator i (o.begin ()); i != o.end (); ++i)
    {
      if (i->empty ())
        r.clear ();
      else
      {
        if (!r.empty ())
          r += ' ';

        r += *i;
      }
    }
  }

  if (m.count ("options"))
  {
    strings const& o (m.get<strings> ("options"));

    for (strings::const_iterator i (o.begin ()); i != o.end (); ++i)
    {
      if (i->empty ())
        r.clear ();
      else
      {
        if (!r.empty ())
          r += ' ';

        r += *i;
      }
    }
  }

  return r;
}

string context::
column_options (semantics::data_member& m, string const& kp)
{
  if (kp.empty ())
    return column_options (m);

  string k (kp + "-options");

  // Accumulate options from type, container, and member.
  //
  semantics::type& c (utype (m));
  semantics::type* t (&utype (m, kp));

  if (semantics::class_* p = object_pointer (*t))
    t = &utype (*id_member (*p));

  if (semantics::type* w = wrapper (*t))
    t = w;

  string r;

  if (t->count ("options"))
  {
    strings const& o (t->get<strings> ("options"));

    for (strings::const_iterator i (o.begin ()); i != o.end (); ++i)
    {
      if (i->empty ())
        r.clear ();
      else
      {
        if (!r.empty ())
          r += ' ';

        r += *i;
      }
    }
  }

  if (c.count (k))
  {
    strings const& o (c.get<strings> (k));

    for (strings::const_iterator i (o.begin ()); i != o.end (); ++i)
    {
      if (i->empty ())
        r.clear ();
      else
      {
        if (!r.empty ())
          r += ' ';

        r += *i;
      }
    }
  }

  if (m.count (k))
  {
    strings const& o (m.get<strings> (k));

    for (strings::const_iterator i (o.begin ()); i != o.end (); ++i)
    {
      if (i->empty ())
        r.clear ();
      else
      {
        if (!r.empty ())
          r += ' ';

        r += *i;
      }
    }
  }

  return r;
}

context::type_map_type::const_iterator context::type_map_type::
find (semantics::type& t, semantics::names* hint)
{
  const_iterator e (end ()), i (e);

  // First check the hinted name. This allows us to handle things like
  // size_t which is nice to map to the same type irrespective of the
  // actual type. Since this type can be an alias for the one we are
  // interested in, go into nested hints.
  //
  for (; hint != 0 && i == e; hint = hint->hint ())
    i = base::find (t.fq_name (hint));

  // If the hinted name didn't work, try the primary name (e.g.,
  // ::std::string) instead of a user typedef (e.g., my_string).
  //
  if (i == e)
    i = base::find (t.fq_name ());

  return i;
}

string context::
database_type_impl (semantics::type& t,
                    semantics::names* hint,
                    bool id,
                    bool* null)
{
  using semantics::enum_;

  // By default map an enum as its underlying type.
  //
  if (enum_* e = dynamic_cast<enum_*> (&t))
    return database_type_impl (
      e->underlying_type (), e->underlying_type_hint (), id, null);

  // Built-in type mapping.
  //
  type_map_type::const_iterator i (data_->type_map_.find (t, hint));
  if (i != data_->type_map_.end ())
  {
    if (null != 0)
      *null = i->second.null;
    return id ? i->second.id_type : i->second.type;
  }

  return string ();
}

static string
public_name_impl (semantics::data_member& m)
{
  string s (m.name ());
  size_t n (s.size ());

  // Do basic processing: remove trailing and leading underscores
  // as well as the 'm_' prefix.
  //
  // @@ What if the resulting names conflict?
  //
  size_t b (0), e (n - 1);

  if (n > 2 && s[0] == 'm' && s[1] == '_')
    b += 2;

  for (; b <= e && s[b] == '_'; b++) ;
  for (; e >= b && s[e] == '_'; e--) ;

  return b > e ? s : string (s, b, e - b + 1);
}

string context::
public_name_db (semantics::data_member& m) const
{
  return public_name_impl (m);
}

string context::
compose_name (string const& prefix, string const& name)
{
  string r (prefix);
  size_t n (r.size ());

  // Add an underscore unless one is already in the prefix or
  // the name is empty. Similarly, remove it if it is there but
  // the name is empty.
  //
  if (n != 0)
  {
    if (r[n - 1] != '_')
    {
      if (!name.empty ())
        r += '_';
    }
    else
    {
      if (name.empty ())
        r.resize (n - 1);
    }
  }

  r += name;
  return r;
}

string context::
transform_name (string const& name, sql_name_type type) const
{
  string r;

  if (!data_->sql_name_regex_[type].empty () ||
      !data_->sql_name_regex_[sql_name_all].empty ())
  {
    bool t (options.sql_name_regex_trace ());

    if (t)
      cerr << "name: '" << name << "'" << endl;

    bool found (false);

    // First try the type-specific transformations, if that didn't work,
    // try common transformations.
    //
    for (unsigned short j (0); !found && j < 2; ++j)
    {
      regex_mapping const& rm = data_->sql_name_regex_[
        j == 0 ? type : sql_name_all];

      for (regex_mapping::const_iterator i (rm.begin ()); i != rm.end (); ++i)
      {
        if (t)
          cerr << "try: '" << i->regex () << "' : ";

        if (i->match (name))
        {
          r = i->replace (name);
          found = true;

          if (t)
            cerr << "'" << r << "' : ";
        }

        if (t)
          cerr << (found ? '+' : '-') << endl;

        if (found)
          break;
      }
    }

    if (!found)
      r = name;
  }
  else
    r = name;

  if (options.sql_name_case ().count (db) != 0)
  {
    switch (options.sql_name_case ()[db])
    {
    case name_case::upper:
      {
        r = data_->sql_name_upper_.replace (r);
        break;
      }
    case name_case::lower:
      {
        r = data_->sql_name_lower_.replace (r);
        break;
      }
    }
  }

  return r;
}

string context::
public_name (semantics::data_member& m, bool e) const
{
  return e ? escape (public_name_impl (m)) : public_name_impl (m);
}

string context::
flat_name (string const& fq)
{
  string r;
  r.reserve (fq.size ());

  for (string::size_type i (0), n (fq.size ()); i < n; ++i)
  {
    char c (fq[i]);

    if (c == ':')
    {
      if (!r.empty ())
        r += '_';
      ++i; // Skip the second ':'.
    }
    else
      r += c;
  }

  return r;
}

string context::
escape (string const& name) const
{
  typedef string::size_type size;

  string r;
  size n (name.size ());

  // In most common cases we will have that many characters.
  //
  r.reserve (n);

  for (size i (0); i < n; ++i)
  {
    char c (name[i]);

    if (i == 0)
    {
      if (!((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            c == '_'))
        r = (c >= '0' && c <= '9') ? "cxx_" : "cxx";
    }

    if (!((c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') ||
          c == '_'))
      r += '_';
    else
      r += c;
  }

  if (r.empty ())
    r = "cxx";

  // Custom reserved words.
  //
  /*
  reserved_name_map_type::const_iterator i (reserved_name_map.find (r));

  if (i != reserved_name_map.end ())
  {
    if (!i->second.empty ())
      return i->second;
    else
      r += L'_';
  }
  */

  // Keywords
  //
  if (keyword_set.find (r) != keyword_set.end ())
  {
    r += '_';

    // Re-run custom words.
    //
    /*
    i = reserved_name_map.find (r);

    if (i != reserved_name_map.end ())
    {
      if (!i->second.empty ())
        return i->second;
      else
        r += L'_';
    }
    */
  }

  return r;
}

string context::
make_guard (string const& s) const
{
  // Split words, e.g., "FooBar" to "Foo_Bar" and convert everything
  // to upper case.
  //
  string r;
  for (string::size_type i (0), n (s.size ()); i < n - 1; ++i)
  {
    char c1 (s[i]);
    char c2 (s[i + 1]);

    r += toupper (c1);

    if (isalpha (c1) && isalpha (c2) && islower (c1) && isupper (c2))
      r += "_";
  }
  r += toupper (s[s.size () - 1]);

  return escape (r);
}

static string
charlit (unsigned int u)
{
  string r ("\\x");
  bool lead (true);

  for (short i (7); i >= 0; --i)
  {
    unsigned int x ((u >> (i * 4)) & 0x0F);

    if (lead)
    {
      if (x == 0)
        continue;

      lead = false;
    }

    r += static_cast<char> (x < 10 ? ('0' + x) : ('A' + x - 10));
  }

  return r;
}

static string
strlit_ascii (string const& str)
{
  string r;
  string::size_type n (str.size ());

  // In most common cases we will have that many chars.
  //
  r.reserve (n + 2);

  r += '"';

  bool escape (false);

  for (string::size_type i (0); i < n; ++i)
  {
    unsigned int u (static_cast<unsigned int> (str[i]));

    // [128 - ]     - unrepresentable
    // 127          - \x7F
    // [32  - 126]  - as is
    // [0   - 31]   - \X or \xXX
    //

    if (u < 32 || u == 127)
    {
      switch (u)
      {
      case '\n':
        {
          r += "\\n";
          break;
        }
      case '\t':
        {
          r += "\\t";
          break;
        }
      case '\v':
        {
          r += "\\v";
          break;
        }
      case '\b':
        {
          r += "\\b";
          break;
        }
      case '\r':
        {
          r += "\\r";
          break;
        }
      case '\f':
        {
          r += "\\f";
          break;
        }
      case '\a':
        {
          r += "\\a";
          break;
        }
      default:
        {
          r += charlit (u);
          escape = true;
          break;
        }
      }
    }
    else if (u < 127)
    {
      if (escape)
      {
        // Close and open the string so there are no clashes.
        //
        r += '"';
        r += '"';

        escape = false;
      }

      switch (u)
      {
      case '"':
        {
          r += "\\\"";
          break;
        }
      case '\\':
        {
          r += "\\\\";
          break;
        }
      default:
        {
          r += static_cast<char> (u);
          break;
        }
      }
    }
    else
    {
      // @@ Unrepresentable character.
      //
      r += '?';
    }
  }

  r += '"';

  return r;
}

string context::
strlit (string const& str)
{
  return strlit_ascii (str);
}

void context::
inst_header (bool decl, bool omit_exp)
{
  if (decl)
  {
    if (!ext.empty ())
      os << ext << " ";
    else
      os << "extern ";
  }

  os << "template struct";

  if (!omit_exp && !exp.empty ())
  {
    // If we are generating an explicit instantiation directive rather
    // than the extern template declaration, then omit the export symbol
    // if we already have it in the header (i.e., extern symbol specified
    // and defined). If we don't do that, then we get GCC warnings saying
    // that the second set of visibility attributes is ignored.
    //
    if (!decl && !ext.empty ())
      os << endl
         << "#ifndef " << ext << endl
         << options.export_symbol ()[db] << endl
         << "#endif" << endl;
    else
      os << " " << exp;
  }
  else
    os << " ";
}

namespace
{
  struct column_count_impl: object_members_base
  {
    column_count_impl (object_section* section = 0)
        : object_members_base (false, section)
    {
    }

    virtual void
    traverse_pointer (semantics::data_member& m, semantics::class_& c)
    {
      // Object pointers in views require special treatment.
      //
      if (view_member (m))
      {
        using semantics::class_;

        column_count_type cc;

        if (class_* root = polymorphic (c))
        {
          // For a polymorphic class we are going to load all the members
          // from all the bases (i.e., equivalent to the first statement
          // in the list of SELECT statements generated for the object).
          // So our count should be the same as the first value in the
          // generated column_counts array.
          //
          for (class_* b (&c);; b = &polymorphic_base (*b))
          {
            column_count_type const& ccb (column_count (*b, section_));

            cc.total += ccb.total - (b != root ? ccb.id : 0);
            cc.separate_load += ccb.separate_load;
            cc.soft += ccb.soft;

            if (b == root)
              break;
          }
        }
        else
          cc = column_count (c, section_);

        c_.total += cc.total - cc.separate_load;

        if (added (member_path_) != 0 || deleted (member_path_) != 0)
          c_.soft += cc.total;
        else
          c_.soft += cc.soft;
      }
      else
      {
        size_t t (c_.total);

        object_members_base::traverse_pointer (m, c);

        if (context::inverse (m))
        {
          size_t n (c_.total - t);

          c_.inverse += n;

          if (separate_update (member_path_))
            c_.separate_update -= n;
        }
      }
    }

    virtual void
    traverse_simple (semantics::data_member& m)
    {
      c_.total++;

      bool ro (context::readonly (member_path_, member_scope_));

      if (id ())
        c_.id++;
      else if (ro)
        c_.readonly++;
      else if (context::version (m))
        c_.optimistic_managed++;

      // For now discriminator can only be a simple value.
      //
      if (discriminator (m))
        c_.discriminator++;

      {
        unsigned long long av (added (member_path_));
        unsigned long long dv (deleted (member_path_));

        // If the addition/deletion version is the same as the section's,
        // then don't count.
        //
        if (user_section* s = dynamic_cast<user_section*> (section_))
        {
          if (av == added (*s->member))
            av = 0;

          if (dv == deleted (*s->member))
            dv = 0;
        }

        if (av != 0)
          c_.added++;

        if (dv != 0)
          c_.deleted++;

        if (av != 0 || dv != 0)
          c_.soft++;
      }

      if (separate_load (member_path_))
        c_.separate_load++;

      if (separate_update (member_path_) && !ro)
        c_.separate_update++;
    }

    context::column_count_type c_;
  };
}

context::column_count_type context::
column_count (semantics::class_& c, object_section* s)
{
  if (s == 0)
  {
    // Whole class.
    //
    if (!c.count ("column-count"))
    {
      column_count_impl t;
      t.traverse (c);
      c.set ("column-count", t.c_);
    }

    return c.get<column_count_type> ("column-count");
  }
  else
  {
    column_count_impl t (s);
    t.traverse (c);
    return t.c_;
  }
}

namespace
{
  struct has_a_impl: object_members_base
  {
    has_a_impl (unsigned short flags, object_section* s)
        : object_members_base ((flags & context::include_base) != 0, s),
          r_ (0),
          flags_ (flags)
    {
    }

    size_t
    result () const
    {
      return r_;
    }

    virtual bool
    section_test (data_member_path const& mp)
    {
      object_section& s (section (mp));

      // Include eager loaded members into the main section if requested.
      //
      return section_ == 0 ||
        *section_ == s ||
        ((flags_ & include_eager_load) != 0 &&
         *section_ == main_section &&
         !s.separate_load ());
    }

    virtual void
    traverse_pointer (semantics::data_member& m, semantics::class_&)
    {
      // Ignore polymorphic id references; they are represented as
      // pointers but are normally handled in a special way.
      //
      if (m.count ("polymorphic-ref"))
        return;

      // Ignore added/deleted members if so requested.
      //
      if (check_soft ())
        return;

      if (context::is_a (member_path_, member_scope_, flags_))
        r_++;

      // No need to go inside.
    }

    virtual void
    traverse_simple (semantics::data_member&)
    {
      // Ignore added/deleted members if so requested.
      //
      if (check_soft ())
        return;

      if (context::is_a (member_path_, member_scope_, flags_))
        r_++;
    }

    virtual void
    traverse_container (semantics::data_member& m, semantics::type&)
    {
      // Ignore added/deleted members if so requested.
      //
      if (check_soft ())
        return;

      // Ignore versioned containers if so requested.
      //
      if ((flags_ & exclude_versioned) != 0 && versioned (m))
        return;

      // We don't cross the container boundaries (separate table).
      //
      unsigned short f (flags_ & (context::test_container |
                                  context::test_straight_container |
                                  context::test_inverse_container |
                                  context::test_readonly_container |
                                  context::test_readwrite_container |
                                  context::test_smart_container));

      if (context::is_a (member_path_,
                         member_scope_,
                         f,
                         context::container_vt (m),
                         "value"))
        r_++;
    }

    virtual void
    traverse_object (semantics::class_& c)
    {
      if ((flags_ & context::exclude_base) == 0)
        inherits (c);

      names (c);
    }

  private:
    bool
    check_soft ()
    {
      if ((flags_ & exclude_added) != 0 || (flags_ & exclude_deleted) != 0)
      {
        unsigned long long av (added (member_path_));
        unsigned long long dv (deleted (member_path_));

        // If the addition/deletion version is the same as the section's,
        // then don't exclude.
        //
        if (user_section* s = dynamic_cast<user_section*> (section_))
        {
          if (av == added (*s->member))
            av = 0;

          if (dv == deleted (*s->member))
            dv = 0;
        }

        if ((av != 0 && (flags_ & exclude_added) != 0) ||
            (dv != 0 && (flags_ & exclude_deleted) != 0))
          return true;
      }

      return false;
    }

  private:
    size_t r_;
    unsigned short flags_;
  };
}

bool context::
is_a (data_member_path const& mp,
      data_member_scope const& ms,
      unsigned short f,
      semantics::type& t,
      string const& kp)
{
  bool r (false);

  semantics::data_member& m (*mp.back ());

  if (f & test_pointer)
    r = r || object_pointer (t);

  if (f & test_eager_pointer)
    r = r || (object_pointer (t) && !lazy_pointer (t));

  if (f & test_lazy_pointer)
    r = r || (object_pointer (t) && lazy_pointer (t));

  semantics::type* c;
  if ((f & (test_container |
            test_straight_container |
            test_inverse_container |
            test_readonly_container |
            test_readwrite_container |
            test_smart_container)) != 0 &&
      (c = container (m)) != 0)
  {
    if (f & test_container)
      r = r || true;

    if (f & test_straight_container)
      r = r || !inverse (m, kp);

    if (f & test_inverse_container)
      r = r || inverse (m, kp);

    if (f & test_readonly_container)
      r = r || readonly (mp, ms);

    if (f & test_readwrite_container)
      r = r || (!inverse (m, kp) && !readonly (mp, ms));

    if (f & test_smart_container)
      r = r || (!inverse (m, kp) && !unordered (m) && container_smart (*c));
  }

  return r;
}

size_t context::
has_a (semantics::class_& c, unsigned short flags, object_section* s)
{
  has_a_impl impl (flags, s);
  impl.dispatch (c);
  return impl.result ();
}

string context::
process_include_path (string const& ip, bool prefix, char open)
{
  bool t (options.include_regex_trace ());
  string p (prefix ? options.include_prefix () : string ());

  if (!p.empty () && p[p.size () - 1] != '/')
    p.append ("/");

  string path (p + ip), r;

  if (t)
    cerr << "include: '" << path << "'" << endl;

  bool found (false);

  for (regex_mapping::const_iterator i (include_regex.begin ());
       i != include_regex.end (); ++i)
  {
    if (t)
      cerr << "try: '" << i->regex () << "' : ";

    if (i->match (path))
    {
      r = i->replace (path);
      found = true;

      if (t)
        cerr << "'" << r << "' : ";
    }

    if (t)
      cerr << (found ? '+' : '-') << endl;

    if (found)
      break;
  }

  if (!found)
    r = path;

  // Add brackets or quotes unless the path already has them.
  //
  if (!r.empty () && r[0] != '"' && r[0] != '<')
  {
    bool b (open == '<' || (open == '\0' && options.include_with_brackets ()));
    char op (b ? '<' : '"'), cl (b ? '>' : '"');
    r = op + r + cl;
  }

  return r;
}
