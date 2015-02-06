// file      : mssql/types/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test SQL Server type conversion.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/exceptions.hxx>
#include <odb/mssql/database.hxx>
#include <odb/mssql/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
namespace mssql = odb::mssql;
using namespace mssql;

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_specific_database<database> (argc, argv));

    {
      object o (1);

      o.bit_ = 1;
      o.utint_ = 222;
      o.stint_ = -123;
      o.usint_ = 65000;
      o.ssint_ = -12345;
      o.uint_ = 4294967290U;
      o.sint_ = -1234567890;
      o.ubint_ = 18446744073709551610ULL;
      o.sbint_ = -1234567890123456789LL;

      o.fsm_ = -214748.3648F;
      o.dsm_ = 214748.3647;
      o.ism_ = -2147483647 -1;

      o.dm1_ = -922337203685477.5808;
      o.dm2_ = 922337203685476.3520; // 922337203685477.5807
      o.im_ = 9223372036854775807LL;

      o.f4_ = 123.123F;
      o.f8_ = 123.1234567;

      o.schar_ = "short data char     ";
      o.svchar_ = "short data varchar";

      o.lchar_.assign (1025, 'a');
      o.lvchar_ = "long data varchar"; // Test the short string optimization.
      o.mvchar_.assign (70000, 'm');
      o.text_.assign (70000, 't');

      o.snchar_ = L"short data nchar\x1FFF\xD7FF  ";
      o.snvchar_ = L"short data nvarchar \x1FFF\xD7FF";

      o.lnchar_.assign (513, L'\x1234');
      o.lnvchar_ = L""; // Test empty string.
      o.mnvchar_.assign (70000, L'\x2345');
      o.ntext_.assign (70000, L'\x4356');

      const char sdata[] = "abc""\x00\x01""def";
      memcpy (o.sbin_, sdata, sizeof (sdata));
      o.svbin_.assign (sdata, sdata + sizeof (sdata));

      string ldata (256 * 1024, '\x01');
      memset (o.lbin_, 2, sizeof (o.lbin_));
      o.lvbin_.assign (50, '\x03');
      o.mvbin_.assign (ldata.begin (), ldata.end ());
      o.image_.assign (ldata.begin (), ldata.end ());

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      o.date_ = date_time (2011, 12, 20, 0, 0, 0, 0, 0, 0);
      o.time7_ = date_time (0, 0, 0, 13, 34, 39, 123456789, 0, 0);
      o.time4_ = date_time (0, 0, 0, 13, 34, 39, 123456700, 0, 0);
#endif
      o.sdt_ = date_time (2011, 12, 20, 15, 44, 29, 123456700, 0, 0);
      o.dt_ = date_time (2011, 12, 20, 15, 44, 29, 123456700, 0, 0);
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      o.dt2_ = date_time (2011, 12, 20, 15, 44, 29, 123456700, 0, 0);
      o.dto7_ = date_time (2011, 12, 20, 15, 44, 29, 123456700, 2, 0);
      o.dto0_ = date_time (2011, 12, 20, 15, 44, 29, 123456700, 2, 0);
#endif

#ifdef _WIN32
      // 6F846D41-C89A-4E4D-B22F-56443CFA543F
      o.guid_.Data1 = 0x6F846D41;
      o.guid_.Data2 = 0xC89A;
      o.guid_.Data3 = 0x4E4D;
      memcpy (&o.guid_.Data4, "\xB2\x2F\x56\x44\x3C\xFA\x54\x3F", 8);
#endif
      memcpy (o.uuid_, "\x6F\x84\x6D\x41\xC8\x9A\x4E\x4D\xB2\x2F"
              "\x56\x44\x3C\xFA\x54\x3F", 16);

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      o.time7_ = date_time (0, 0, 0, 13, 34, 39, 123456700, 0, 0);
      o.time4_ = date_time (0, 0, 0, 13, 34, 39, 123400000, 0, 0);
#endif
      o.sdt_ = date_time (2011, 12, 20, 15, 44, 0, 0, 0, 0);
      o.dt_ = date_time (2011, 12, 20, 15, 44, 29, 123000000, 0, 0);
#if !defined(MSSQL_SERVER_VERSION) || MSSQL_SERVER_VERSION >= 1000
      o.dto0_ = date_time (2011, 12, 20, 15, 44, 29, 0, 2, 0);
#endif

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<object> o1 (db->load<object> (1));
        t.commit ();

        assert (o == *o1);
      }

      // Test short/long data in queries.
      //
      typedef mssql::query<object> query;
      typedef odb::result<object> result;

      {
        transaction t (db->begin ());

        {
          result r (db->query<object> (query::svchar == o.svchar_));
          assert (size (r) == 1);
        }

        {
          result r (db->query<object> (query::snvchar == o.snvchar_));
          assert (size (r) == 1);
        }

        {
          result r (db->query<object> (query::mvchar == o.mvchar_));
          assert (size (r) == 1);
        }

        {
          result r (db->query<object> (query::mnvchar == o.mnvchar_));
          assert (size (r) == 1);
        }

        t.commit ();
      }
    }

    // Test long NULL data.
    //
    {
      long_null o1 (1);
      long_null o2 (2);
      o2.str_.reset (new string);
      o2.str_->assign (70000, 'x');

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<long_null> p1 (db->load<long_null> (1));
        auto_ptr<long_null> p2 (db->load<long_null> (2));
        t.commit ();

        assert (o1 == *p1);
        assert (o2 == *p2);
      }
    }

    // Test long data in containers.
    //
    {
      long_cont o (1);
      o.v.push_back (long_comp ("aaa", 123));
      o.v.push_back (long_comp (string (500, 'b'), 234));
      o.v.push_back (long_comp (string (70000, 'c'), 345));

      // Persist.
      //
      {
        transaction t (db->begin ());
        db->persist (o);
        t.commit ();
      }

      // Load.
      //
      {
        transaction t (db->begin ());
        auto_ptr<long_cont> p (db->load<long_cont> (1));
        t.commit ();

        assert (o == *p);
      }
    }

    // Test char/wchar_t arrays.
    //
    {
      char_array o1 (1, "", L"");
      char_array o2 (2, "1234567890", L"12345678\x1FFF\xD7FF");
      char_array o3 (3, "1234567890123456", L"12345678901234\x1FFF\xD7FF");

      {
        transaction t (db->begin ());
        db->persist (o1);
        db->persist (o2);
        db->persist (o3);
        t.commit ();
      }

      // SQL Server returns padded values for CHAR(N)/NCHAR(N).
      //
      memcpy (o1.s2, "                ", 16);
      o1.s3[0] = o1.c1 = ' ';
      memcpy (o2.s2, "1234567890      ", 16);

      memset (o1.ls2, ' ', 1025);
      memset (o2.ls2 + 10, ' ', 1025 - 10);

      memcpy (o1.ws2, L"                ", 16 * sizeof (wchar_t));
      o1.ws3[0] = o1.wc1 = L' ';
      memcpy (o2.ws2, L"12345678\x1FFF\xD7FF      ", 16 * sizeof (wchar_t));

      for (size_t i (0); i < 257; ++i)
        o1.lws2[i] = L' ';

      for (size_t i (10); i < 257; ++i)
        o2.lws2[i] = L' ';

      {
        transaction t (db->begin ());
        auto_ptr<char_array> p1 (db->load<char_array> (1));
        auto_ptr<char_array> p2 (db->load<char_array> (2));
        auto_ptr<char_array> p3 (db->load<char_array> (3));
        t.commit ();

        assert (o1 == *p1);
        assert (o2 == *p2);
        assert (o3 == *p3);
      }
    }

    // Test optimistic concurrency using ROWVERSION.
    //
    {
      rowversion o (123);
      o.str = "abc";

      {
        transaction t (db->begin ());
        db->persist (o);
        assert (o.ver != 0);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<rowversion> p (db->load<rowversion> (o.id_));
        assert (p->ver == o.ver);
        p->str += 'd';
        db->update (*p);
        assert (p->ver > o.ver);

        // Double-check object version was updated.
        //
        {
          auto_ptr<rowversion> p1 (db->load<rowversion> (o.id_));
          assert (p->ver == p1->ver);
        }

        o.str += 'D';
        try
        {
          db->update (o);
          assert (false);
        }
        catch (const odb::object_changed&) {}
        db->reload (o);
        assert (o.ver == p->ver);
        o.str += 'D';
        db->update (o);
        t.commit ();
      }
    }

    {
      rowversion_auto o;
      o.str = "abc";

      {
        transaction t (db->begin ());
        db->persist (o);
        assert (o.ver != 0);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<rowversion_auto> p (db->load<rowversion_auto> (o.id_));
        assert (p->ver == o.ver);
        p->str += 'd';
        db->update (*p);
        assert (p->ver > o.ver);
        o.str += 'D';
        try
        {
          db->update (o);
          assert (false);
        }
        catch (const odb::object_changed&) {}
        db->reload (o);
        assert (o.ver == p->ver);
        o.str += 'D';
        db->update (o);
        t.commit ();
      }
    }
  }
  catch (const odb::exception& e)
  {
    cerr << e.what () << endl;
    return 1;
  }
}
