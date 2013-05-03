PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE kexi__objectdata (o_id Integer UNSIGNED NOT NULL, o_data CLOB, o_sub_id Text);
INSERT INTO "kexi__objectdata" VALUES(2,'<!DOCTYPE EXTENDED_TABLE_SCHEMA>
<EXTENDED_TABLE_SCHEMA version="2">
 <field name="owner">
  <lookup-column>
   <row-source>
    <type>table</type>
    <name>persons</name>
   </row-source>
   <bound-column>
    <number>0</number>
   </bound-column>
   <visible-column>
    <number>3</number>
   </visible-column>
  </lookup-column>
 </field>
</EXTENDED_TABLE_SCHEMA>
','extended_schema');
CREATE TABLE kexi__db (db_property Text(32), db_value CLOB);
INSERT INTO "kexi__db" VALUES('kexidb_major_ver','1');
INSERT INTO "kexi__db" VALUES('kexidb_minor_ver','9');
INSERT INTO "kexi__db" VALUES('kexiproject_major_ver','1');
INSERT INTO "kexi__db" VALUES(' kexiproject_major_ver','Project major version');
INSERT INTO "kexi__db" VALUES('kexiproject_minor_ver','0');
INSERT INTO "kexi__db" VALUES(' kexiproject_minor_ver','Project minor version');
CREATE TABLE kexi__fields (t_id Integer UNSIGNED, f_type Byte UNSIGNED, f_name Text, f_length Integer, f_precision Integer, f_constraints Integer, f_options Integer, f_default Text, f_order Integer, f_caption Text, f_help CLOB);
INSERT INTO "kexi__fields" VALUES(1,3,'id',0,0,119,1,NULL,0,'ID',NULL);
INSERT INTO "kexi__fields" VALUES(1,3,'age',0,0,0,1,NULL,1,'Age',NULL);
INSERT INTO "kexi__fields" VALUES(1,11,'name',0,0,0,0,NULL,2,'Name',NULL);
INSERT INTO "kexi__fields" VALUES(1,11,'surname',0,0,0,0,NULL,3,'Surname',NULL);
INSERT INTO "kexi__fields" VALUES(2,3,'id',0,0,119,1,NULL,0,'ID',NULL);
INSERT INTO "kexi__fields" VALUES(2,3,'owner',0,0,0,1,NULL,1,'Car owner',NULL);
INSERT INTO "kexi__fields" VALUES(2,11,'model',0,0,0,0,NULL,2,'Car model',NULL);
CREATE TABLE kexi__objects (o_id INTEGER PRIMARY KEY, o_type Byte UNSIGNED, o_name Text, o_caption Text, o_desc CLOB);
INSERT INTO "kexi__objects" VALUES(1,1,'persons','Persons in our factory',NULL);
INSERT INTO "kexi__objects" VALUES(2,1,'cars','Cars owned by persons',NULL);
CREATE TABLE persons (id INTEGER PRIMARY KEY, age Integer UNSIGNED, name Text, surname Text);
INSERT INTO "persons" VALUES(1,27,'Jaroslaw','Staniek');
INSERT INTO "persons" VALUES(2,60,'Lech','Walesa');
INSERT INTO "persons" VALUES(3,45,'Bill','Gates');
INSERT INTO "persons" VALUES(4,35,'John','Smith');
CREATE TABLE cars (id INTEGER PRIMARY KEY, owner Integer UNSIGNED, model Text);
INSERT INTO "cars" VALUES(1,1,'Fiat');
INSERT INTO "cars" VALUES(2,2,'Syrena');
INSERT INTO "cars" VALUES(3,3,'Chrysler');
INSERT INTO "cars" VALUES(4,3,'BMW');
INSERT INTO "cars" VALUES(5,2,'Volvo');
CREATE TABLE kexi__blobs (o_id INTEGER PRIMARY KEY, o_data BLOB, o_name Text, o_caption Text, o_mime Text NOT NULL, o_folder_id Integer UNSIGNED);
CREATE TABLE kexi__parts (p_id INTEGER PRIMARY KEY, p_name Text, p_mime Text, p_url Text);
INSERT INTO "kexi__parts" VALUES(1,'Tables','kexi/table','org.kexi-project.table');
INSERT INTO "kexi__parts" VALUES(2,'Queries','kexi/query','org.kexi-project.query');
INSERT INTO "kexi__parts" VALUES(3,'Forms','kexi/form','org.kexi-project.form');
INSERT INTO "kexi__parts" VALUES(4,'Reports','kexi/report','org.kexi-project.report');
INSERT INTO "kexi__parts" VALUES(5,'Scripts','kexi/script','org.kexi-project.script');
INSERT INTO "kexi__parts" VALUES(6,'Web pages','kexi/web','org.kexi-project.web');
INSERT INTO "kexi__parts" VALUES(7,'Macros','kexi/macro','org.kexi-project.macro');
CREATE TABLE kexi__userdata (d_user Text NOT NULL, o_id INTEGER PRIMARY KEY, d_sub_id Text NOT NULL, d_data CLOB);
INSERT INTO "kexi__userdata" VALUES('',1,'columnWidths','120,120,120,120');
INSERT INTO "kexi__userdata" VALUES('',2,'columnWidths','120,120,120');
COMMIT;
