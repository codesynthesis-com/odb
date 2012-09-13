/* This file contains custom type definitions and helper functions.
 */

SET FEEDBACK OFF;
WHENEVER SQLERROR EXIT FAILURE;
WHENEVER OSERROR EXIT FAILURE;

-- @@ Temporary workaround: we cannot replace a type if there are
--    tables that use it. So need to drop the tables first, then
--    create/replace the type, and then create the tables.
--
--CREATE OR REPLACE TYPE Numbers AS VARRAY(100) OF NUMBER(10);

BEGIN
  BEGIN
    EXECUTE IMMEDIATE 'CREATE TYPE Numbers AS VARRAY(100) OF NUMBER(10)';
  EXCEPTION
    WHEN OTHERS THEN
      IF SQLCODE != -955 THEN RAISE; END IF;
  END;
END;
/

CREATE OR REPLACE FUNCTION string_to_numbers(in_str IN VARCHAR2) RETURN Numbers
IS
  ret          Numbers := Numbers();
  s_pos        NUMBER := 1;
  e_pos        NUMBER := 0;
BEGIN
   IF in_str IS NOT NULL THEN
     LOOP
       e_pos := INSTR(in_str, ',', s_pos);
       EXIT WHEN e_pos = 0;
       ret.extend;
       ret(ret.COUNT) := CAST(SUBSTR(in_str, s_pos, e_pos - s_pos) AS NUMBER);
       s_pos := e_pos + 1;
     END LOOP;
     ret.extend;
     ret(ret.COUNT) := CAST(SUBSTR(in_str, s_pos) AS NUMBER);
   END IF;
   RETURN ret;
END;
/

CREATE OR REPLACE FUNCTION numbers_to_string(in_nums IN Numbers) RETURN VARCHAR2
IS
  ret          VARCHAR2(1500);
BEGIN
  IF in_nums.COUNT != 0 THEN
    FOR i IN in_nums.FIRST .. in_nums.LAST LOOP
      IF i != in_nums.FIRST THEN
        ret := ret || ',';
      END IF;
      ret := ret || in_nums(i);
    END LOOP;
  END IF;
  RETURN ret;
END;
/

EXIT;
