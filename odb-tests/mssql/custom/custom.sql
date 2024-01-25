/* This file contains helper functions.
 */

IF OBJECT_ID('dbo.variant_to_string', 'FN') IS NOT NULL
  DROP FUNCTION dbo.variant_to_string;
GO

IF OBJECT_ID('dbo.string_to_variant', 'FN') IS NOT NULL
  DROP FUNCTION dbo.string_to_variant;
GO

CREATE FUNCTION dbo.variant_to_string (@val SQL_VARIANT) RETURNS VARCHAR(max)
AS
BEGIN
  RETURN CAST(SQL_VARIANT_PROPERTY(@val, 'BaseType') AS SYSNAME) + ' ' +
    CAST(@val AS VARCHAR(max))
END;
GO

CREATE FUNCTION dbo.string_to_variant (@val VARCHAR(max)) RETURNS SQL_VARIANT
AS
BEGIN
  DECLARE @ret SQL_VARIANT

  DECLARE @pos BIGINT
  DECLARE @vtype SYSNAME
  DECLARE @vtext VARCHAR(max)

  SET @pos = CHARINDEX(' ', @val)
  SET @vtype = SUBSTRING(@val, 1, @pos - 1)
  SET @vtext = SUBSTRING(@val, @pos + 1, LEN(@val))

       IF @vtype = 'tinyint'  SET @ret = CAST(@vtext AS TINYINT)
  ELSE IF @vtype = 'smallint' SET @ret = CAST(@vtext AS SMALLINT)
  ELSE IF @vtype = 'int'      SET @ret = CAST(@vtext AS INT)
  ELSE IF @vtype = 'bigint'   SET @ret = CAST(@vtext AS BIGINT)
  ELSE IF @vtype = 'char'     SET @ret = CAST(@vtext AS CHAR(8000))
  ELSE IF @vtype = 'varchar'  SET @ret = CAST(@vtext AS VARCHAR(8000))

  RETURN @ret
END;
GO
