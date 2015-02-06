// file      : common/section/polymorphism/driver.cxx
// copyright : Copyright (c) 2009-2015 Code Synthesis Tools CC
// license   : GNU GPL v2; see accompanying LICENSE file

// Test sections in polymorphic objects.
//

#include <memory>   // std::auto_ptr
#include <cassert>
#include <iostream>

#include <odb/session.hxx>
#include <odb/database.hxx>
#include <odb/transaction.hxx>

#include <common/common.hxx>

#include "test.hxx"
#include "test-odb.hxx"

using namespace std;
using namespace odb::core;

struct failed {};

int
main (int argc, char* argv[])
{
  try
  {
    auto_ptr<database> db (create_database (argc, argv));

    // Test basic polymorphic section functionality.
    //
    {
      using namespace test1;

      base b (123, "abc");
      derived d (234, "bcd", true);

      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (d);
        t.commit ();

        assert (b.rs1.loaded ());
        assert (b.rs2.loaded ());
        assert (b.rs3.loaded ());
        assert (b.rs4.loaded ());
        assert (b.bs1.loaded ());

        assert (d.rs1.loaded ());
        assert (d.rs2.loaded ());
        assert (d.rs3.loaded ());
        assert (d.rs4.loaded ());
        assert (d.bs1.loaded ());
        assert (d.ds1.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        assert (!pb->rs1.loaded ());
        assert (!pb->rs2.loaded ());
        assert (!pb->rs3.loaded ());
        assert (!pb->rs4.loaded ());
        assert (!pb->bs1.loaded ());

        assert (!pd->rs1.loaded ());
        assert (!pd->rs2.loaded ());
        assert (!pd->rs3.loaded ());
        assert (!pd->rs4.loaded ());
        assert (!pd->bs1.loaded ());
        assert (!pd->ds1.loaded ());

        assert (pb->rs1n != b.rs1n && pb->rs1s != b.rs1s &&
                pb->rs2n != b.rs2n && pb->rs2v != b.rs2v &&
                pb->rs3v != b.rs3v &&
                pb->rs4n != b.rs4n &&
                pb->bs1n != b.bs1n);

        assert (pd->rs1n != d.rs1n && pd->rs1s != d.rs1s &&
                  pd->rs1b != d.rs1b && pd->rs1v != d.rs1v &&
                pd->rs2n != d.rs2n && pd->rs2v != d.rs2v &&
                pd->rs3v != d.rs3v && pd->rs3n != d.rs3n &&
                pd->rs4n != d.rs4n && pd->rs4s != d.rs4s &&
                pd->bs1n != d.bs1n && pd->bs1s != d.bs1s &&
                pd->ds1n != d.ds1n);

        db->load (*pb, pb->rs1);
        db->load (*pb, pb->rs2);
        db->load (*pb, pb->rs3);
        db->load (*pb, pb->rs4);
        db->load (*pb, pb->bs1);

        root* pr (pd.get ());
        db->load (*pr, pr->rs1); // Via base.
        db->load (*pd, pd->rs2);
        db->load (*pr, pr->rs3); // Via base.
        db->load (*pd, pd->rs4);
        db->load (*pd, pd->bs1);
        db->load (*pd, pd->ds1);

        try
        {
          db->load (*pr, pd->bs1); // Object-section association is static.
          assert (false);
        }
        catch (const section_not_in_object&) {}

        assert (pb->rs1.loaded ());
        assert (pb->rs2.loaded ());
        assert (pb->rs3.loaded ());
        assert (pb->rs4.loaded ());
        assert (pb->bs1.loaded ());

        assert (pd->rs1.loaded ());
        assert (pd->rs2.loaded ());
        assert (pd->rs3.loaded ());
        assert (pd->rs4.loaded ());
        assert (pd->bs1.loaded ());
        assert (pd->ds1.loaded ());

        assert (pb->rs1n == b.rs1n && pb->rs1s == b.rs1s &&
                pb->rs2n == b.rs2n && pb->rs2v == b.rs2v &&
                pb->rs3v == b.rs3v &&
                pb->rs4n == b.rs4n &&
                pb->bs1n == b.bs1n);

        assert (pd->rs1n == d.rs1n && pd->rs1s == d.rs1s &&
                  pd->rs1b == d.rs1b && pd->rs1v == d.rs1v &&
                pd->rs2n == d.rs2n && pd->rs2v == d.rs2v &&
                pd->rs3v == d.rs3v && pd->rs3n == d.rs3n &&
                pd->rs4n == d.rs4n && pd->rs4s == d.rs4s &&
                pd->bs1n == d.bs1n && pd->bs1s == d.bs1s &&
                pd->ds1n == d.ds1n);
        t.commit ();
      }

      // Update object.
      //
      b.rs1n++;
      b.rs1s += 'd';
      b.rs1.change ();
      b.rs2n++;
      b.rs2v[0]++;
      b.rs3v[0]++;
      b.rs4n++;
      b.bs1n++;

      d.rs1n++;
      d.rs1s += 'e';
      d.rs1b = !d.rs1b;
      d.rs1v[0]++;
      d.rs1.change ();
      d.rs2n++;
      d.rs2v[0]++;
      d.rs3v[0]++;
      d.rs3n++;
      d.rs4n++;
      d.rs4s += 'e';
      d.bs1n++;
      d.bs1s += 'e';
      d.ds1n++;

      {
        transaction t (db->begin ());
        db->update (b);
        db->update (d);
        t.commit ();

        assert (!b.rs1.changed ());
        assert (!d.rs1.changed ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->load (*pb, pb->rs1);
        db->load (*pb, pb->rs2);
        db->load (*pb, pb->rs3);
        db->load (*pb, pb->rs4);
        db->load (*pb, pb->bs1);

        db->load (*pd, pd->rs1);
        db->load (*pd, pd->rs2);
        db->load (*pd, pd->rs3);
        db->load (*pd, pd->rs4);
        db->load (*pd, pd->bs1);
        db->load (*pd, pd->ds1);

        assert (pb->rs1n == b.rs1n && pb->rs1s == b.rs1s &&
                pb->rs2n == b.rs2n && pb->rs2v == b.rs2v &&
                pb->rs3v == b.rs3v &&
                pb->rs4n == b.rs4n &&
                pb->bs1n == b.bs1n);

        assert (pd->rs1n == d.rs1n && pd->rs1s == d.rs1s &&
                  pd->rs1b == d.rs1b && pd->rs1v == d.rs1v &&
                pd->rs2n == d.rs2n && pd->rs2v == d.rs2v &&
                pd->rs3v == d.rs3v && pd->rs3n == d.rs3n &&
                pd->rs4n == d.rs4n && pd->rs4s == d.rs4s &&
                pd->bs1n == d.bs1n && pd->bs1s == d.bs1s &&
                pd->ds1n == d.ds1n);
        t.commit ();
      }

      // Update section.
      //
      b.rs1n++;
      b.rs1s += 'd';
      b.rs2n++;
      b.rs2v[0]++;
      b.rs3v[0]++;
      b.rs4n++;
      b.bs1n++;

      d.rs1n++;
      d.rs1s += 'e';
      d.rs1b = !d.rs1b;
      d.rs1v[0]++;
      d.rs2n++;
      d.rs2v[0]++;
      d.rs3v[0]++;
      d.rs3n++;
      d.rs4n++;
      d.rs4s += 'e';
      d.bs1n++;
      d.bs1s += 'e';
      d.ds1n++;

      {
        transaction t (db->begin ());
        db->update (b, b.rs1);
        db->update (b, b.rs2);
        db->update (b, b.rs3);
        db->update (b, b.rs4);
        db->update (b, b.bs1);

        db->update (d, d.rs1);
        db->update (d, d.rs2);
        db->update (d, d.rs3);
        db->update (d, d.rs4);
        db->update (d, d.bs1);
        db->update (d, d.ds1);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->load (*pb, pb->rs1);
        db->load (*pb, pb->rs2);
        db->load (*pb, pb->rs3);
        db->load (*pb, pb->rs4);
        db->load (*pb, pb->bs1);

        db->load (*pd, pd->rs1);
        db->load (*pd, pd->rs2);
        db->load (*pd, pd->rs3);
        db->load (*pd, pd->rs4);
        db->load (*pd, pd->bs1);
        db->load (*pd, pd->ds1);

        assert (pb->rs1n == b.rs1n && pb->rs1s == b.rs1s &&
                pb->rs2n == b.rs2n && pb->rs2v == b.rs2v &&
                pb->rs3v == b.rs3v &&
                pb->rs4n == b.rs4n &&
                pb->bs1n == b.bs1n);

        assert (pd->rs1n == d.rs1n && pd->rs1s == d.rs1s &&
                  pd->rs1b == d.rs1b && pd->rs1v == d.rs1v &&
                pd->rs2n == d.rs2n && pd->rs2v == d.rs2v &&
                pd->rs3v == d.rs3v && pd->rs3n == d.rs3n &&
                pd->rs4n == d.rs4n && pd->rs4s == d.rs4s &&
                pd->bs1n == d.bs1n && pd->bs1s == d.bs1s &&
                pd->ds1n == d.ds1n);
        t.commit ();
      }

      // Reload.
      //
      b.rs1n++;
      b.rs1s += 'd';
      b.rs1.change ();
      b.rs2n++;
      b.rs2v[0]++;
      b.rs3v[0]++;
      b.rs4n++;
      b.bs1n++;

      d.rs1n++;
      d.rs1s += 'e';
      d.rs1b = !d.rs1b;
      d.rs1v[0]++;
      d.rs1.change ();
      d.rs2n++;
      d.rs2v[0]++;
      d.rs3v[0]++;
      d.rs3n++;
      d.rs4n++;
      d.rs4s += 'e';
      d.bs1n++;
      d.bs1s += 'e';
      d.ds1n++;

      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->load (*pb, pb->rs1);
        db->load (*pb, pb->rs2);
        db->load (*pb, pb->rs3);
        db->load (*pb, pb->rs4);
        db->load (*pb, pb->bs1);

        db->load (*pd, pd->rs1);
        db->load (*pd, pd->rs2);
        db->load (*pd, pd->rs3);
        db->load (*pd, pd->rs4);
        db->load (*pd, pd->bs1);
        db->load (*pd, pd->ds1);

        db->update (b);
        db->update (d);

        db->reload (*pb);
        db->reload (*pd);

        assert (pb->rs1n == b.rs1n && pb->rs1s == b.rs1s &&
                pb->rs2n == b.rs2n && pb->rs2v == b.rs2v &&
                pb->rs3v == b.rs3v &&
                pb->rs4n == b.rs4n &&
                pb->bs1n == b.bs1n);

        assert (pd->rs1n == d.rs1n && pd->rs1s == d.rs1s &&
                  pd->rs1b == d.rs1b && pd->rs1v == d.rs1v &&
                pd->rs2n == d.rs2n && pd->rs2v == d.rs2v &&
                pd->rs3v == d.rs3v && pd->rs3n == d.rs3n &&
                pd->rs4n == d.rs4n && pd->rs4s == d.rs4s &&
                pd->bs1n == d.bs1n && pd->bs1s == d.bs1s &&
                pd->ds1n == d.ds1n);

        t.commit ();
      }
    }

    // Test empty section and override "gap".
    //
    {
      using namespace test2;

      derived d (234);

      {
        transaction t (db->begin ());
        db->persist (d);
        t.commit ();

        assert (d.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        assert (!pd->s.loaded ());
        assert (pd->sn != d.sn && pd->sv != d.sv);

        root* pr (pd.get ());
        db->load (*pr, pr->s); // Via root.

        assert (pd->s.loaded ());
        assert (pd->sn == d.sn && pd->sv == d.sv);
        t.commit ();
      }

      // Update object.
      //
      d.sn++;
      d.sv[0]++;

      {
        transaction t (db->begin ());
        root* pr (&d);
        db->update (pr); // Via root.
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        base* pb (pd.get ());
        db->load (*pb, pb->s); // Via base.

        assert (pd->sn == d.sn && pd->sv == d.sv);
        t.commit ();
      }

      // Update section.
      //
      d.sn++;
      d.sv[0]++;

      {
        transaction t (db->begin ());
        root* pr (&d);
        db->update (*pr, pr->s); // Via root.
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->load (*pd, pd->s);

        assert (pd->sn == d.sn && pd->sv == d.sv);
        t.commit ();
      }

      // Reload.
      //
      d.sn++;
      d.sv[0]++;

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->load (*pd, pd->s);

        db->update (d);

        root* pr (pd.get ());
        db->reload (*pr);

        assert (pd->sn == d.sn && pd->sv == d.sv);
        t.commit ();
      }
    }

    // Test value-only/container-only base/override combinations.
    //
    {
      using namespace test3;

      root r (123);
      base b (234);
      derived d (345, "abc");

      {
        transaction t (db->begin ());
        db->persist (r);
        db->persist (b);
        db->persist (d);
        t.commit ();

        assert (r.s1.loaded ());
        assert (r.s2.loaded ());
        assert (r.s3.loaded ());
        assert (r.s4.loaded ());

        assert (b.s1.loaded ());
        assert (b.s2.loaded ());
        assert (b.s3.loaded ());
        assert (b.s4.loaded ());

        assert (d.s1.loaded ());
        assert (d.s2.loaded ());
        assert (d.s3.loaded ());
        assert (d.s4.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<root> pr (db->load<root> (r.id));
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        assert (!pr->s1.loaded ());
        assert (!pr->s2.loaded ());
        assert (!pr->s3.loaded ());
        assert (!pr->s4.loaded ());

        assert (!pb->s1.loaded ());
        assert (!pb->s2.loaded ());
        assert (!pb->s3.loaded ());
        assert (!pb->s4.loaded ());

        assert (!pd->s1.loaded ());
        assert (!pd->s2.loaded ());
        assert (!pd->s3.loaded ());
        assert (!pd->s4.loaded ());

        assert (pr->s1n != r.s1n &&
                pr->s2n != r.s2n &&
                pr->s3v != r.s3v &&
                pr->s4nv != r.s4nv);

        assert (pb->s1n != b.s1n &&
                pb->s2n != b.s2n &&
                pb->s3v != b.s3v &&
                pb->s4nv != b.s4nv);

        assert (pd->s1n != d.s1n && pd->s1s != d.s1s &&
                pd->s2n != d.s2n && pd->s2v != d.s2v &&
                pd->s3v != d.s3v && pd->s3n != d.s3n &&
                pd->s4nv != d.s4nv && pd->s4sv != d.s4sv);

        db->load (*pr, pr->s1);
        db->load (*pr, pr->s2);
        db->load (*pr, pr->s3);
        db->load (*pr, pr->s4);

        db->load (*pb, pb->s1);
        db->load (*pb, pb->s2);
        db->load (*pb, pb->s3);
        db->load (*pb, pb->s4);

        root* pdr (pd.get ());
        db->load (*pdr, pdr->s1);
        db->load (*pdr, pdr->s2);
        db->load (*pdr, pdr->s3);
        db->load (*pdr, pdr->s4);

        assert (pr->s1.loaded ());
        assert (pr->s2.loaded ());
        assert (pr->s3.loaded ());
        assert (pr->s4.loaded ());

        assert (pb->s1.loaded ());
        assert (pb->s2.loaded ());
        assert (pb->s3.loaded ());
        assert (pb->s4.loaded ());

        assert (pd->s1.loaded ());
        assert (pd->s2.loaded ());
        assert (pd->s3.loaded ());
        assert (pd->s4.loaded ());

        assert (pr->s1n == r.s1n &&
                pr->s2n == r.s2n &&
                pr->s3v == r.s3v &&
                pr->s4nv == r.s4nv);

        assert (pb->s1n == b.s1n &&
                pb->s2n == b.s2n &&
                pb->s3v == b.s3v &&
                pb->s4nv == b.s4nv);

        assert (pd->s1n == d.s1n && pd->s1s == d.s1s &&
                pd->s2n == d.s2n && pd->s2v == d.s2v &&
                pd->s3v == d.s3v && pd->s3n == d.s3n &&
                pd->s4nv == d.s4nv && pd->s4sv == d.s4sv);
        t.commit ();
      }

      // Update object.
      //
      r.s1n++;
      r.s2n++;
      r.s3v[0]++;
      r.s4nv[0]++;

      b.s1n++;
      b.s2n++;
      b.s3v[0]++;
      b.s4nv[0]++;

      d.s1n++;
      d.s1s += 'd';
      d.s2n++;
      d.s2v[0]++;
      d.s3v[0]++;
      d.s3n++;
      d.s4nv[0]++;
      d.s4sv[0] += 'd';

      {
        transaction t (db->begin ());
        db->update (r);
        db->update (b);
        db->update (d);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<root> pr (db->load<root> (r.id));
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->load (*pr, pr->s1);
        db->load (*pr, pr->s2);
        db->load (*pr, pr->s3);
        db->load (*pr, pr->s4);

        db->load (*pb, pb->s1);
        db->load (*pb, pb->s2);
        db->load (*pb, pb->s3);
        db->load (*pb, pb->s4);

        db->load (*pd, pd->s1);
        db->load (*pd, pd->s2);
        db->load (*pd, pd->s3);
        db->load (*pd, pd->s4);

        assert (pr->s1n == r.s1n &&
                pr->s2n == r.s2n &&
                pr->s3v == r.s3v &&
                pr->s4nv == r.s4nv);

        assert (pb->s1n == b.s1n &&
                pb->s2n == b.s2n &&
                pb->s3v == b.s3v &&
                pb->s4nv == b.s4nv);

        assert (pd->s1n == d.s1n && pd->s1s == d.s1s &&
                pd->s2n == d.s2n && pd->s2v == d.s2v &&
                pd->s3v == d.s3v && pd->s3n == d.s3n &&
                pd->s4nv == d.s4nv && pd->s4sv == d.s4sv);
        t.commit ();
      }

      // Update section.
      //
      r.s1n++;
      r.s2n++;
      r.s3v[0]++;
      r.s4nv[0]++;

      b.s1n++;
      b.s2n++;
      b.s3v[0]++;
      b.s4nv[0]++;

      d.s1n++;
      d.s1s += 'd';
      d.s2n++;
      d.s2v[0]++;
      d.s3v[0]++;
      d.s3n++;
      d.s4nv[0]++;
      d.s4sv[0] += 'd';

      {
        transaction t (db->begin ());
        db->update (r, r.s1);
        db->update (r, r.s2);
        db->update (r, r.s3);
        db->update (r, r.s4);

        db->update (b, b.s1);
        db->update (b, b.s2);
        db->update (b, b.s3);
        db->update (b, b.s4);

        root& rr (d);
        db->update (rr, rr.s1);
        db->update (rr, rr.s2);
        db->update (rr, rr.s3);
        db->update (rr, rr.s4);
        t.commit ();
      }

      {
        transaction t (db->begin ());
        auto_ptr<root> pr (db->load<root> (r.id));
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->load (*pr, pr->s1);
        db->load (*pr, pr->s2);
        db->load (*pr, pr->s3);
        db->load (*pr, pr->s4);

        db->load (*pb, pb->s1);
        db->load (*pb, pb->s2);
        db->load (*pb, pb->s3);
        db->load (*pb, pb->s4);

        db->load (*pd, pd->s1);
        db->load (*pd, pd->s2);
        db->load (*pd, pd->s3);
        db->load (*pd, pd->s4);

        assert (pr->s1n == r.s1n &&
                pr->s2n == r.s2n &&
                pr->s3v == r.s3v &&
                pr->s4nv == r.s4nv);

        assert (pb->s1n == b.s1n &&
                pb->s2n == b.s2n &&
                pb->s3v == b.s3v &&
                pb->s4nv == b.s4nv);

        assert (pd->s1n == d.s1n && pd->s1s == d.s1s &&
                pd->s2n == d.s2n && pd->s2v == d.s2v &&
                pd->s3v == d.s3v && pd->s3n == d.s3n &&
                pd->s4nv == d.s4nv && pd->s4sv == d.s4sv);
        t.commit ();
      }

      // Reload.
      //
      r.s1n++;
      r.s2n++;
      r.s3v[0]++;
      r.s4nv[0]++;

      b.s1n++;
      b.s2n++;
      b.s3v[0]++;
      b.s4nv[0]++;

      d.s1n++;
      d.s1s += 'd';
      d.s2n++;
      d.s2v[0]++;
      d.s3v[0]++;
      d.s3n++;
      d.s4nv[0]++;
      d.s4sv[0] += 'd';

      {
        transaction t (db->begin ());
        auto_ptr<root> pr (db->load<root> (r.id));
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->load (*pr, pr->s1);
        db->load (*pr, pr->s2);
        db->load (*pr, pr->s3);
        db->load (*pr, pr->s4);

        db->load (*pb, pb->s1);
        db->load (*pb, pb->s2);
        db->load (*pb, pb->s3);
        db->load (*pb, pb->s4);

        db->load (*pd, pd->s1);
        db->load (*pd, pd->s2);
        db->load (*pd, pd->s3);
        db->load (*pd, pd->s4);

        db->update (r);
        db->update (b);
        db->update (d);

        db->reload (*pr);
        db->reload (*pb);
        db->reload (*pd);

        assert (pr->s1n == r.s1n &&
                pr->s2n == r.s2n &&
                pr->s3v == r.s3v &&
                pr->s4nv == r.s4nv);

        assert (pb->s1n == b.s1n &&
                pb->s2n == b.s2n &&
                pb->s3v == b.s3v &&
                pb->s4nv == b.s4nv);

        assert (pd->s1n == d.s1n && pd->s1s == d.s1s &&
                pd->s2n == d.s2n && pd->s2v == d.s2v &&
                pd->s3v == d.s3v && pd->s3n == d.s3n &&
                pd->s4nv == d.s4nv && pd->s4sv == d.s4sv);
        t.commit ();
      }
    }

    // Test basic polymorphic optimistic section functionality.
    //
    {
      using namespace test4;

      base b (123, "abc");
      derived d (234, "bcd", true);

      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (d);
        t.commit ();

        assert (b.rs1.loaded ());
        assert (b.rs2.loaded ());
        assert (b.rs3.loaded ());
        assert (b.rs4.loaded ());
        assert (b.bs1.loaded ());

        assert (d.rs1.loaded ());
        assert (d.rs2.loaded ());
        assert (d.rs3.loaded ());
        assert (d.rs4.loaded ());
        assert (d.bs1.loaded ());
        assert (d.ds1.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        assert (!pb->rs1.loaded ());
        assert (!pb->rs2.loaded ());
        assert (!pb->rs3.loaded ());
        assert (!pb->rs4.loaded ());
        assert (!pb->bs1.loaded ());

        assert (!pd->rs1.loaded ());
        assert (!pd->rs2.loaded ());
        assert (!pd->rs3.loaded ());
        assert (!pd->rs4.loaded ());
        assert (!pd->bs1.loaded ());
        assert (!pd->ds1.loaded ());

        assert (pb->rs1n != b.rs1n && pb->rs1s != b.rs1s &&
                pb->rs2n != b.rs2n &&
                pb->rs3n != b.rs3n &&
                pb->rs4n != b.rs4n && pb->rs4s != b.rs4s);

        assert (pd->rs1n != d.rs1n && pd->rs1s != d.rs1s &&
                  pd->rs1b != d.rs1b && pd->rs1v != d.rs1v &&
                pd->rs2n != d.rs2n &&
                pd->rs3n != d.rs3n && pd->rs3s != d.rs3s &&
                pd->rs4n != d.rs4n && pd->rs4s != d.rs4s &&
                  pd->rs4v != d.rs4v &&
                pd->bs1n != d.bs1n &&
                pd->ds1v != d.ds1v);

        db->load (*pb, pb->rs1);
        db->load (*pb, pb->rs2);
        db->load (*pb, pb->rs3);
        db->load (*pb, pb->rs4);
        db->load (*pb, pb->bs1); // No-op.

        root* pr (pd.get ());
        db->load (*pr, pr->rs1); // Via base.
        db->load (*pd, pd->rs2);
        db->load (*pd, pd->rs3);
        db->load (*pr, pr->rs4); // Via base.
        db->load (*pd, pd->bs1);
        db->load (*pd, pd->ds1);

        assert (pb->rs1.loaded ());
        assert (pb->rs2.loaded ());
        assert (pb->rs3.loaded ());
        assert (pb->rs4.loaded ());
        assert (pb->bs1.loaded ());

        assert (pd->rs1.loaded ());
        assert (pd->rs2.loaded ());
        assert (pd->rs3.loaded ());
        assert (pd->rs4.loaded ());
        assert (pd->bs1.loaded ());
        assert (pd->ds1.loaded ());

        assert (pb->rs1n == b.rs1n && pb->rs1s == b.rs1s &&
                pb->rs2n == b.rs2n &&
                pb->rs3n == b.rs3n &&
                pb->rs4n == b.rs4n && pb->rs4s == b.rs4s);

        assert (pd->rs1n == d.rs1n && pd->rs1s == d.rs1s &&
                  pd->rs1b == d.rs1b && pd->rs1v == d.rs1v &&
                pd->rs2n == d.rs2n &&
                pd->rs3n == d.rs3n && pd->rs3s == d.rs3s &&
                pd->rs4n == d.rs4n && pd->rs4s == d.rs4s &&
                  pd->rs4v == d.rs4v &&
                pd->bs1n == d.bs1n &&
                pd->ds1v == d.ds1v);
        t.commit ();
      }

      base b1 (b);
      derived d1 (d);

      // Update object.
      //
      for (unsigned short s (1); s < 7; ++s)
      {
        b1.rs1n++;
        b1.rs1s += 'd';
        b1.rs1.change ();
        b1.rs4s += 'd';

        d1.rs1n++;
        d1.rs1s += 'e';
        d1.rs1b = !d.rs1b;
        d1.rs1v[0]++;
        d1.rs1.change ();
        d1.rs4s += 'e';
        d1.rs4v[0]++;
        d1.bs1n++;
        d1.ds1v[0]++;

        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->update (b1);
        db->update (d1);

        assert (!b1.rs1.changed ());
        assert (!d1.rs1.changed ());

        assert (b.v != b1.v);
        assert (d.v != d1.v);

        try
        {
          bool a (false);
          switch (s)
          {
          case 1: db->load (*pb, pb->rs1); break;
          case 2: db->load (*pb, pb->rs2); break;
          case 3: db->load (*pb, pb->rs3); break;
          case 4: db->load (*pb, pb->rs4); break;
          case 5:
          case 6: a = true; break; // No-op.
          default: break;
          }
          assert (a);
        }
        catch (const object_changed&)
        {
          db->reload (*pb);

          assert (!pb->rs1.loaded ());
          assert (!pb->rs2.loaded ());
          assert (!pb->rs3.loaded ());
          assert (!pb->rs3.loaded ());
          assert (!pb->bs1.loaded ());

          db->load (*pb, pb->rs1);
          db->load (*pb, pb->rs2);
          db->load (*pb, pb->rs3);
          db->load (*pb, pb->rs4);
          db->load (*pb, pb->bs1); // No-op.

          assert (pb->rs1n == b1.rs1n && pb->rs1s == b1.rs1s &&
                  pb->rs2n == b1.rs2n &&
                  pb->rs3n == b1.rs3n &&
                  pb->rs4n == b1.rs4n && pb->rs4s == b1.rs4s);
        }

        try
        {
          switch (s)
          {
          case 1: db->load (*pd, pd->rs1); break;
          case 2: db->load (*pd, pd->rs2); break;
          case 3: db->load (*pd, pd->rs3); break;
          case 4: db->load (*pd, pd->rs4); break;
          case 5: db->load (*pd, pd->bs1); break;
          case 6: db->load (*pd, pd->ds1); break;
          default: break;
          }
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*pd);

          assert (!pd->rs1.loaded ());
          assert (!pd->rs2.loaded ());
          assert (!pd->rs3.loaded ());
          assert (!pd->rs4.loaded ());
          assert (!pd->bs1.loaded ());
          assert (!pd->ds1.loaded ());

          db->load (*pd, pd->rs1);
          db->load (*pd, pd->rs2);
          db->load (*pd, pd->rs3);
          db->load (*pd, pd->rs4);
          db->load (*pd, pd->bs1);
          db->load (*pd, pd->ds1);

          assert (pd->rs1n == d1.rs1n && pd->rs1s == d1.rs1s &&
                    pd->rs1b == d1.rs1b && pd->rs1v == d1.rs1v &&
                  pd->rs2n == d1.rs2n &&
                  pd->rs3n == d1.rs3n && pd->rs3s == d1.rs3s &&
                  pd->rs4n == d1.rs4n && pd->rs4s == d1.rs4s &&
                    pd->rs4v == d1.rs4v &&
                  pd->bs1n == d1.bs1n &&
                  pd->ds1v == d1.ds1v);
        }

        db->reload (b);
        db->reload (d);

        assert (b.v == b1.v);
        assert (d.v == d1.v);

        assert (b.rs1n == b1.rs1n && b.rs1s == b1.rs1s &&
                b.rs2n == b1.rs2n &&
                b.rs3n == b1.rs3n &&
                b.rs4n == b1.rs4n && b.rs4s == b1.rs4s);

        assert (d.rs1n == d1.rs1n && d.rs1s == d1.rs1s &&
                  d.rs1b == d1.rs1b && d.rs1v == d1.rs1v &&
                d.rs2n == d1.rs2n &&
                d.rs3n == d1.rs3n && d.rs3s == d1.rs3s &&
                d.rs4n == d1.rs4n && d.rs4s == d1.rs4s &&
                  d.rs4v == d1.rs4v &&
                d.bs1n == d1.bs1n &&
                d.ds1v == d1.ds1v);

        t.commit ();
      }

      // Update section.
      //
      for (unsigned short s (1); s < 7; ++s)
      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        switch (s)
        {
        case 1:
          b1.rs1n++;
          b1.rs1s += 'd';

          d1.rs1n++;
          d1.rs1s += 'e';
          d1.rs1b = !d.rs1b;
          d1.rs1v[0]++;

          db->update (b1, b1.rs1);
          db->update (d1, d1.rs1);

          assert (b.v != b1.v);
          assert (d.v != d1.v);
          break;
        case 2:
          db->update (b1, b1.rs2); // No-op.
          db->update (d1, d1.rs2); // No-op.

          assert (b.v == b1.v);
          assert (d.v == d1.v);
          continue; // Object hasn't changed.
        case 3:
          db->update (b1, b1.rs3); // No-op.
          db->update (d1, d1.rs3); // No-op.

          assert (b.v == b1.v);
          assert (d.v == d1.v);
          continue; // Object hasn't changed.
        case 4:
          b1.rs4s += 'd';

          d1.rs4s += 'e';
          d1.rs4v[0]++;

          db->update (b1, b1.rs4);
          db->update (d1, d1.rs4);

          assert (b.v != b1.v);
          assert (d.v != d1.v);
          break;
        case 5:
          d1.bs1n++;

          db->update (b1, b1.bs1); // No-op.
          db->update (d1, d1.bs1);

          assert (b.v == b1.v);
          assert (d.v != d1.v);
          break;
        case 6:
          d1.ds1v[0]++;

          db->update (d1, d1.ds1);

          assert (d.v != d1.v);
          break;
        default: break;
        }

        try
        {
          bool a (false);
          switch (s)
          {
          case 1: db->load (*pb, pb->rs1); break;
          case 4: db->load (*pb, pb->rs4); break;
          case 5:
          case 6: a = true; break; // No-op.
          default: break;
          }
          assert (a);
        }
        catch (const object_changed&)
        {
          db->reload (*pb);

          assert (!pb->rs1.loaded ());
          assert (!pb->rs2.loaded ());
          assert (!pb->rs3.loaded ());
          assert (!pb->rs4.loaded ());
          assert (!pb->bs1.loaded ());

          db->load (*pb, pb->rs1);
          db->load (*pb, pb->rs2);
          db->load (*pb, pb->rs3);
          db->load (*pb, pb->rs4);
          db->load (*pb, pb->bs1); // No-op.

          assert (pb->rs1n == b1.rs1n && pb->rs1s == b1.rs1s &&
                  pb->rs2n == b1.rs2n &&
                  pb->rs3n == b1.rs3n &&
                  pb->rs4n == b1.rs4n && pb->rs4s == b1.rs4s);
        }

        try
        {
          switch (s)
          {
          case 1: db->load (*pd, pd->rs1); break;
          case 4: db->load (*pd, pd->rs4); break;
          case 5: db->load (*pd, pd->bs1); break;
          case 6: db->load (*pd, pd->ds1); break;
          default: break;
          }
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*pd);

          assert (!pd->rs1.loaded ());
          assert (!pd->rs2.loaded ());
          assert (!pd->rs3.loaded ());
          assert (!pd->rs4.loaded ());
          assert (!pd->bs1.loaded ());
          assert (!pd->ds1.loaded ());

          db->load (*pd, pd->rs1);
          db->load (*pd, pd->rs2);
          db->load (*pd, pd->rs3);
          db->load (*pd, pd->rs4);
          db->load (*pd, pd->bs1);
          db->load (*pd, pd->ds1);

          assert (pd->rs1n == d1.rs1n && pd->rs1s == d1.rs1s &&
                    pd->rs1b == d1.rs1b && pd->rs1v == d1.rs1v &&
                  pd->rs2n == d1.rs2n &&
                  pd->rs3n == d1.rs3n && pd->rs3s == d1.rs3s &&
                  pd->rs4n == d1.rs4n && pd->rs4s == d1.rs4s &&
                    pd->rs4v == d1.rs4v &&
                  pd->bs1n == d1.bs1n &&
                  pd->ds1v == d1.ds1v);
        }

        db->reload (b);
        db->reload (d);

        assert (b.v == b1.v);
        assert (d.v == d1.v);

        assert (b.rs1n == b1.rs1n && b.rs1s == b1.rs1s &&
                b.rs2n == b1.rs2n &&
                b.rs3n == b1.rs3n &&
                b.rs4n == b1.rs4n && b.rs4s == b1.rs4s);

        assert (d.rs1n == d1.rs1n && d.rs1s == d1.rs1s &&
                  d.rs1b == d1.rs1b && d.rs1v == d1.rs1v &&
                d.rs2n == d1.rs2n &&
                d.rs3n == d1.rs3n && d.rs3s == d1.rs3s &&
                d.rs4n == d1.rs4n && d.rs4s == d1.rs4s &&
                  d.rs4v == d1.rs4v &&
                d.bs1n == d1.bs1n &&
                d.ds1v == d1.ds1v);

        t.commit ();
      }

      // Update changed section.
      //
      for (unsigned short s (1); s < 7; ++s)
      {
        if (s == 2 || s == 3) // Readonly sections.
          continue;

        transaction t (db->begin ());

        switch (s)
        {
        case 1:
          b1.rs1n++;
          b1.rs1s += 'd';

          d1.rs1n++;
          d1.rs1s += 'e';
          d1.rs1b = !d.rs1b;
          d1.rs1v[0]++;

          db->update (b1, b1.rs1);
          db->update (d1, d1.rs1);
          break;
        case 4:
          b1.rs4s += 'd';

          d1.rs4s += 'e';
          d1.rs4v[0]++;

          db->update (b1, b1.rs4);
          db->update (d1, d1.rs4);
          break;
        case 5:
          d1.bs1n++;

          db->update (b1, b1.bs1); // No-op.
          db->update (d1, d1.bs1);
          break;
        case 6:
          d1.ds1v[0]++;

          db->update (d1, d1.bs1);
          break;
        default: break;
        }

        try
        {
          bool a (false);
          switch (s)
          {
          case 1: db->update (b, b.rs1); break;
          case 4: db->update (b, b.rs4); break;
          case 5:
          case 6: a = true; break; // No-op.
          default: break;
          }
          assert (a);
        }
        catch (const object_changed&)
        {
          db->reload (b);

          switch (s)
          {
          case 1: db->update (b, b.rs1); break;
          case 4: db->update (b, b.rs4); break;
          default: break;
          }
        }

        try
        {
          switch (s)
          {
          case 1: db->update (d, d.rs1); break;
          case 4: db->update (d, d.rs4); break;
          case 5: db->update (d, d.bs1); break;
          case 6: db->update (d, d.ds1); break;
          default: break;
          }
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (d);

          switch (s)
          {
          case 1: db->update (d, d.rs1); break;
          case 4: db->update (d, d.rs4); break;
          case 5: db->update (d, d.bs1); break;
          case 6: db->update (d, d.ds1); break;
          default: break;
          }
        }

        db->reload (b1);
        db->reload (d1);

        t.commit ();
      }
    }

    // Test polymorphic optimistic readonly/empty to readwrite section
    // override.
    //
    {
      using namespace test5;

      base b;
      derived d (123);

      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (d);
        t.commit ();

        assert (b.s.loaded ());
        assert (d.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        assert (!pb->s.loaded ());
        assert (!pd->s.loaded ());
        assert (pd->sn != d.sn);

        db->load (*pb, pb->s); // No-op.
        db->load (*pd, pd->s);

        assert (pb->s.loaded ());
        assert (pd->s.loaded ());

        assert (pd->sn == d.sn);

        t.commit ();
      }

      // Update object.
      //
      base b1 (b);
      derived d1 (d);
      d1.sn++;
      d1.s.change ();

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->update (d1);

        assert (!d1.s.changed ());
        assert (d.v != d1.v);

        try
        {
          db->load (*pd, pd->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*pd);
          assert (!pd->s.loaded ());
          db->load (*pd, pd->s);
          assert (pd->sn == d1.sn);
        }

        db->reload (d);
        assert (d.v == d1.v);
        assert (d.sn == d1.sn);
        t.commit ();
      }

      // Update section.
      //
      d1.sn++;

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->update (b1, b1.s); // No-op.
        db->update (d1, d1.s);

        assert (b.v == b1.v);
        assert (d.v != d1.v);

        try
        {
          db->load (*pd, pd->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*pd);
          assert (!pd->s.loaded ());
          db->load (*pd, pd->s);

          assert (pd->sn == d1.sn);
        }

        db->reload (d);
        assert (d.v == d1.v);
        assert (d.sn == d1.sn);
        t.commit ();
      }

      // Update changed section.
      //
      d1.sn++;

      {
        transaction t (db->begin ());
        db->update (d1, d1.s);

        try
        {
          db->update (d, d.s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (d);
          db->update (d, d.s);
        }

        t.commit ();
      }
    }

    // Test polymorphic optimistic readonly/empty to readwrite section
    // override, eager-loaded case.
    //
    {
      using namespace test6;

      derived d (123);

      {
        transaction t (db->begin ());
        db->persist (d);
        t.commit ();

        assert (d.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        assert (pd->s.loaded ());
        assert (pd->sn == d.sn);

        t.commit ();
      }

      // Update object.
      //
      derived d1 (d);
      d1.sn++;
      d1.s.change ();

      {
        transaction t (db->begin ());

        db->update (d1);

        assert (!d1.s.changed ());
        assert (d.v != d1.v);

        db->reload (d);
        assert (d.v == d1.v);
        assert (d.sn == d1.sn);
        t.commit ();
      }

      // Update section.
      //
      d1.sn++;

      {
        transaction t (db->begin ());

        db->update (d1, d1.s);
        assert (d.v != d1.v);

        db->reload (d);
        assert (d.v == d1.v);
        assert (d.sn == d1.sn);
        t.commit ();
      }

      // Update changed section.
      //
      d1.sn++;

      {
        transaction t (db->begin ());
        db->update (d1, d1.s);

        try
        {
          db->update (d, d.s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (d);
          db->update (d, d.s);
        }

        t.commit ();
      }
    }

    // Test polymorphic optimistic section added in derived.
    //
    {
      using namespace test7;

      base b;
      derived d (123);

      {
        transaction t (db->begin ());
        db->persist (b);
        db->persist (d);
        t.commit ();

        assert (b.s.loaded ());
        assert (d.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<base> pb (db->load<base> (b.id));
        auto_ptr<derived> pd (db->load<derived> (d.id));

        assert (!pb->s.loaded ());
        assert (!pd->s.loaded ());
        assert (pd->sn != d.sn);

        db->load (*pb, pb->s); // No-op.
        db->load (*pd, pd->s);

        assert (pb->s.loaded ());
        assert (pd->s.loaded ());

        assert (pd->sn == d.sn);

        t.commit ();
      }

      // Update object.
      //
      base b1 (b);
      derived d1 (d);
      d1.sn++;
      d1.s.change ();

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->update (d1);

        assert (!d1.s.changed ());
        assert (d.v != d1.v);

        try
        {
          db->load (*pd, pd->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*pd);
          assert (!pd->s.loaded ());
          db->load (*pd, pd->s);
          assert (pd->sn == d1.sn);
        }

        db->reload (d);
        assert (d.v == d1.v);
        assert (d.sn == d1.sn);
        t.commit ();
      }

      // Update section.
      //
      d1.sn++;

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->update (b1, b1.s); // No-op.
        db->update (d1, d1.s);

        assert (b.v == b1.v);
        assert (d.v != d1.v);

        try
        {
          db->load (*pd, pd->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*pd);
          assert (!pd->s.loaded ());
          db->load (*pd, pd->s);

          assert (pd->sn == d1.sn);
        }

        db->reload (d);
        assert (d.v == d1.v);
        assert (d.sn == d1.sn);
        t.commit ();
      }

      // Update changed section.
      //
      d1.sn++;

      {
        transaction t (db->begin ());
        db->update (d1, d1.s);

        try
        {
          db->update (d, d.s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (d);
          db->update (d, d.s);
        }

        t.commit ();
      }
    }

    // Test reuse/polymorphic inheritance and optimistic mix.
    //
    {
      using namespace test8;

      derived d (123);

      {
        transaction t (db->begin ());
        db->persist (d);
        t.commit ();

        assert (d.s.loaded ());
      }

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        assert (!pd->s.loaded ());
        assert (pd->sn != d.sn);

        db->load (*pd, pd->s);

        assert (pd->s.loaded ());
        assert (pd->sn == d.sn);

        t.commit ();
      }

      // Update object.
      //
      derived d1 (d);
      d1.sn++;
      d1.s.change ();

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->update (d1);

        assert (!d1.s.changed ());
        assert (d.v != d1.v);

        try
        {
          db->load (*pd, pd->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*pd);
          assert (!pd->s.loaded ());
          db->load (*pd, pd->s);
          assert (pd->sn == d1.sn);
        }

        db->reload (d);
        assert (d.v == d1.v);
        assert (d.sn == d1.sn);
        t.commit ();
      }

      // Update section.
      //
      d1.sn++;

      {
        transaction t (db->begin ());
        auto_ptr<derived> pd (db->load<derived> (d.id));

        db->update (d1, d1.s);
        assert (d.v != d1.v);

        try
        {
          db->load (*pd, pd->s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (*pd);
          assert (!pd->s.loaded ());
          db->load (*pd, pd->s);

          assert (pd->sn == d1.sn);
        }

        db->reload (d);
        assert (d.v == d1.v);
        assert (d.sn == d1.sn);
        t.commit ();
      }

      // Update changed section.
      //
      d1.sn++;

      {
        transaction t (db->begin ());
        db->update (d1, d1.s);

        try
        {
          db->update (d, d.s);
          assert (false);
        }
        catch (const object_changed&)
        {
          db->reload (d);
          db->update (d, d.s);
        }

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
