#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <vector>
#include <string>
using std::vector;
using std::string;

//以十六进制方式打印字符串
static int printh(const unsigned char* buf, int len)
{
	int i;
	printf("0X");
	for (i = 0; i < len; i++)
	{
		printf("%02X", buf[i]);
		//if ((i + 2) % 20 == 0)
			//printf("\n");
	}
	//if ((len + 2) % 20 != 0)
		//printf("\n");
	return len;
}

//sqlite3_exec()回调函数
//打印sqlite3_exec()的返回结果
static int callback(void* in, int argc, char** argv, char** ColName)
{
	int i;
	printf("enter callback function\n");
	printf("sqlite3_exec return data is\n");
	for (i = 0; i < argc; i++)
	{
		printf("%s\t", ColName[i]);
	}
	printf("\n");
	for (i = 0; i < argc; i++)
	{
		printf("%s\t", argv[i]);
	}
	printf("\n");
	return 0;
}

//打印数据库的特定表格
static int printTable(sqlite3 *db, const char* tableName, FILE* file = stdout)
{
	int ncols = 0;
	int lines = 0;
	int i;
	int value_ret = 0;
	char sql[100] = { 0 };
	printf("准备打印数据库的%s表\n", tableName);
	sqlite3_stmt *stmt;
	sprintf(sql, "select * from %s;", tableName);
	sqlite3_prepare(db, sql, -1, &stmt, NULL);

	//获取表的列数
	ncols = sqlite3_column_count(stmt);

	value_ret = sqlite3_step(stmt);

	//打印每一列的列名
	for (i = 0; i < ncols; i++)
	{
		printf("%s\t", sqlite3_column_name(stmt, i));
	}
	printf("\b\n");

	//打印每一列的类型
	for (i = 0; i < ncols; i++)
	{
		switch (sqlite3_column_type(stmt, i))
		{
		case SQLITE_INTEGER:
			printf("INT\t");
			break;
		case SQLITE_FLOAT:
			printf("FLOAT\t");
			break;
		case SQLITE_TEXT:
			printf("TEXT\t");
			break;
		case SQLITE_BLOB:
			printf("BLOB\t");
			break;
		case SQLITE_NULL:
			printf("NULL\t");
			break;
		default:
			printf("\nerror in %d\n", __LINE__);
			break;
		}
	}
	printf("\b\n");

	//打印数据
	while (value_ret == SQLITE_ROW)
	{
		lines++;
		for (i = 0; i < ncols; i++)
		{
			//根据类型确定打印参数
			switch (sqlite3_column_type(stmt, i))
			{
			case SQLITE_INTEGER:
				printf("%d\t", sqlite3_column_int(stmt, i));
				break;
			case SQLITE_FLOAT:
				printf("%f\t", sqlite3_column_double(stmt, i));
				break;
			case SQLITE_TEXT:
				printf("%s\t", sqlite3_column_text(stmt, i));
				break;
			case SQLITE_BLOB:
				printh((unsigned char*)sqlite3_column_blob(stmt, i), sqlite3_column_bytes(stmt, i));
				break;
			case SQLITE_NULL:
				printf("NULL\t");
				break;
			default:
				printf("\nerror in %d\n", __LINE__);
				break;
			}
		}
		printf("\b\n");
		//准备处理下一行
		value_ret = sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);
	printf("lines = %d\n", lines);
	return lines;
}

//从文件数据库拷贝到内存数据库（加快速度）,ifSave = 0
//或者从内存数据库拷贝到文件数据库（备份）, ifSave = 1
static int db_backup(sqlite3* memorydb, int ifSave)
{
	int value_ret;
	sqlite3 *filedb;
	sqlite3_backup *backupdb;
	sqlite3 *dbFrom;
	sqlite3 *dbTo;

	//打开一个文件数据库
	value_ret = sqlite3_open("testDatabase.db", &filedb);
	if (value_ret != SQLITE_OK)
	{
		fprintf(stderr, "%d Can't open database:%s\n", __LINE__, sqlite3_errmsg(filedb));
		sqlite3_close(filedb);
		return -1;
	}
	dbFrom = ifSave ? memorydb : filedb;
	dbTo = ifSave ? filedb : memorydb;

	backupdb = sqlite3_backup_init(dbTo, "main", dbFrom, "main");
	if (backupdb)
	{
		sqlite3_backup_step(backupdb, -1);
		sqlite3_backup_finish(backupdb);
		sqlite3_close(filedb);
		return 0;
	}
	else
	{
		sqlite3_close(filedb); 
		sqlite3_backup_finish(backupdb);
		return -1;
	}
}


int main(int argc, char* argv[])
{
	int value_ret = 0;
	double double_val;
	int i;
	int tableNumber = 0;
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char* sql;
	char* sqlErrMsg;
	//测试用的blob向量
	char blobValue[20] = { 0x31,0x32,0x33,0x34,0x35,\
		0x36,0x37,0x38,0x39,0x3a,\
		0x3b,0x3b,0x3d,0x3e,0x3f,\
		0x40,0x41,0x42,0x43,0x44 };
	vector<string> tableName;


	//打开或者新建数据库
	value_ret = sqlite3_open(":memory:", &db);
	if (value_ret != SQLITE_OK)
	{
		fprintf(stderr, "%d Can't open database:%s\n", __LINE__, sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}


	//创建表
	sql = "create table testTable("\
		"ID INT PRIMAEY KEY,"\
		"DOUBLE_VALUE DOUBLE,"\
		"TEXT_VALUE TEXT,"\
		"BLOB_VALUE BLOB);";
	value_ret = sqlite3_exec(db, sql, NULL, NULL, &sqlErrMsg);
	if (value_ret != SQLITE_OK)
	{
		fprintf(stderr, "%d SQL error: %s\n", __LINE__, sqlErrMsg);
		sqlite3_free(sqlErrMsg);
	}


	//插入数据
	sql = "insert into testTable values("\
		"1,1.01,'string1',?);";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_blob(stmt, 1, (void *)blobValue, 20, SQLITE_STATIC);
	sqlite3_step(stmt);
	//sqlite3_reset(stmt)也可以
	sqlite3_finalize(stmt);
	//插入数据
	sql = "insert into testTable values("\
		"1,2.01,'string2',?);";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_blob(stmt, 1, (void *)&blobValue[5], 10, SQLITE_STATIC);
	sqlite3_step(stmt);
	sqlite3_reset(stmt);
	//插入数据
	sql = "insert into testTable values("\
		"1,3.01,'string3',?);";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	//SQLITE_STATIC和SQLITE_TRANSIENT都可以
	sqlite3_bind_blob(stmt, 1, (void *)&blobValue[10], 5, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_reset(stmt);
	//插入数据
	sql = "insert into testTable values("\
		"1,4.01,'string4',?);";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_blob(stmt, 1, (void *)&blobValue[15], 5, SQLITE_TRANSIENT);
	sqlite3_step(stmt);
	sqlite3_reset(stmt);
	//插入数据
	sql = "insert into testTable values("\
		"1,5.01,'string5',?);";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_blob(stmt, 1, (void *)blobValue, 4, SQLITE_STATIC);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	//插入数据
	sql = "insert into testTable values("\
		"1,NULL,NULL,?);";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_blob(stmt, 1, (void *)blobValue, 6, SQLITE_STATIC);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);




	//获取表中的数据
	sql = "select * from testTable;";
	value_ret = sqlite3_exec(db, sql, callback, NULL, &sqlErrMsg);
	if (value_ret != SQLITE_OK)
	{
		fprintf(stderr, "%d SQL error: %s\n", __LINE__, sqlErrMsg);
		sqlite3_free(sqlErrMsg);
	}


	//通过自定义函数打印表格
	printf("\n\n添加数据后，表的内容为\n");
	printTable(db, "testTable");


	//更新表格中的内容
	sql = "UPDATE testTable set ID = 2 Where TEXT_VALUE = 'string2';"\
		"UPDATE testTable set ID = 3 Where TEXT_VALUE = 'string3';"\
		"UPDATE testTable set ID = 4 Where DOUBLE_VALUE = 4.01;"\
		"UPDATE testTable set ID = 6,DOUBLE_Value=6.01, TEXT_value = 'string6' Where TEXT_VALUE IS NULL;";
	value_ret = sqlite3_exec(db, sql, callback, NULL, &sqlErrMsg);
	if (value_ret != SQLITE_OK)
	{
		fprintf(stderr, "%d SQL error: %s\n", __LINE__, sqlErrMsg);
		sqlite3_free(sqlErrMsg);
	}
	//通过blob类型值进行查找并更新表
	sql = "UPDATE testTable set ID = 5 Where BLOB_VALUE = ?;";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_bind_blob(stmt, 1, (void*)blobValue, 4, SQLITE_STATIC);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);


	//通过自定义函数打印表格
	printf("\n\n更新数据后，表的内容为\n");
	printTable(db, "testTable");


	//查找表格中的内容
	sql = "select min(double_value), max(double_value) from testTable;";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	value_ret = sqlite3_step(stmt);
	while (value_ret == SQLITE_ROW)
		value_ret = sqlite3_step(stmt);
	double_val = sqlite3_column_double(stmt, 0);
	printf("\n\n最小的double值为: %f\t ", double_val);
	double_val = sqlite3_column_double(stmt, 1);
	printf("最大的double值为: %f\n\n", double_val);
	sqlite3_finalize(stmt);
	

	//查找表格中不存在的项
	sql = "select min(double_value) from testTable where ID>7;";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	value_ret = sqlite3_step(stmt);
	while(value_ret == SQLITE_ROW)
		value_ret = sqlite3_step(stmt);
	double_val = sqlite3_column_double(stmt, 0);
	printf("\n\n最小的double值为: %f\t ", double_val);
	double_val = sqlite3_column_double(stmt, 1);
	printf("最大的double值为: %f\n\n", double_val);
	sqlite3_finalize(stmt);


	//删除一行
	sql = "delete from testTable where ID = 3;";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	sqlite3_step(stmt);
	sqlite3_finalize(stmt);
	//删除一行
	//sql = "delete from testTable where BLOB_VALUE = ?;";
	//sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	//sqlite3_bind_blob(stmt, 1, (void*)blobValue, 4, SQLITE_STATIC);
	//sqlite3_step(stmt);
	//sqlite3_finalize(stmt);


	//通过自定义函数打印表格
	printTable(db, "testTable");


	//打印数据库中所有的表
	//获取所有的表名
	sql = "select name from sqlite_master where type = 'table'";
	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
	value_ret = sqlite3_step(stmt);
	while (value_ret == SQLITE_ROW)
	{
		tableNumber++;
		tableName.push_back(string((char*)sqlite3_column_text(stmt, 0)));
		value_ret = sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);
	//打印所有的表
	printf("删除几行后,数据库为\n");
	printf("打印数据库中所有的表\n");
	for (i = 0; i < tableNumber; i++)
		printTable(db, tableName[i].c_str());


	//删除表格
	//sql = "drop table testTable;";
	//value_ret = sqlite3_exec(db, sql, callback, NULL, &sqlErrMsg);
	//if (value_ret != SQLITE_OK)
	//{
	//	fprintf(stderr, "%d SQL error: %s\n", __LINE__, sqlErrMsg);
	//	sqlite3_free(sqlErrMsg);
	//}

	//备份数据库
	db_backup(db, 1);

	sqlite3_close(db);
	getchar();
	return 0;
}















/*
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

int main(int argc, char* argv[])
{
	int rc, i, ncols, id, age;
	double salary;
	const char *sql;
	const unsigned char* name;
	const unsigned char* address;
	sqlite3 *db;
	sqlite3_stmt *stmt;

	sqlite3_open("test.db", &db);
	sql = "select * from company;";

	sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	ncols = sqlite3_column_count(stmt);
	rc = sqlite3_step(stmt);

	fprintf(stdout, "column num = %d\n", sqlite3_column_count(stmt));
	for (i = 0; i < ncols; i++)
	{
		fprintf(stdout, "Column: name = %s, storage class = %d, declared = %s\n",
			sqlite3_column_name(stmt, i),
			sqlite3_column_type(stmt, i),
			sqlite3_column_decltype(stmt, i));
	}
	fprintf(stdout, "\n");
	for (i = 0; i < sqlite3_column_count(stmt); i++)
	{
		fprintf(stdout, "%s\t", sqlite3_column_name(stmt, i));
	}
	fprintf(stdout, "\n");
	while (rc == SQLITE_ROW)
	{
		id = sqlite3_column_int(stmt, 0);
		name = sqlite3_column_text(stmt, 1);
		age = sqlite3_column_int(stmt, 2);
		address = sqlite3_column_text(stmt, 3);
		salary = sqlite3_column_double(stmt, 4);
		printf("%d\t%s\t%d\t%s\t%f\n", id, name ? name : (const unsigned char*)"NULL", age,
			address ? address : (const unsigned char*)"NULL", salary);
		rc = sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);


	getchar();
	return 0;
}

*/





/*
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

int main(int argc, char* argv[])
{
	int rc, i, ncols;
	sqlite3 *db;
	sqlite3_stmt *stmt;
	const char *sql;
	const char* tail;

	rc = sqlite3_open("test.db", &db);
	if (rc)
	{
		fprintf(stderr, "Can't open database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		exit(1);
	}

	sql = "select * from company;";
	rc = sqlite3_prepare_v2(db, sql, -1, &stmt, &tail);
	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error:%s\n", sqlite3_errmsg(db));
	}
	rc = sqlite3_step(stmt);
	ncols = sqlite3_column_count(stmt);

	while (rc == SQLITE_ROW)
	{
		for (i = 0; i < ncols; i++)
		{
			fprintf(stdout, "'%s' ", sqlite3_column_text(stmt, i));
		}
		fprintf(stdout, "\n");
		rc = sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	getchar();
	return 0;
}
*/










//连接数据库
/*
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

int main(int argc, char* argv[])
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;

	rc = sqlite3_open("test.db", &db);

	if (rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(1);
	}
	else {
		fprintf(stderr, "Opened database successfully\n");
	}
	sqlite3_close(db);
	getchar();
}
*/




//创建表
/*
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

static int callback(void* NotUsed, int argc, char** argv, char** ColName)
{
int i;
for (i = 0; i < argc; i++)
{
printf("%s = %s\n", ColName[i], argv[i] ? argv[i] : "NULL");
}
printf("\n");
return 0;
}

int main(int argc, char* argv[])
{
sqlite3 *db;
char *zErrMsg = NULL;
int rc;
const char *sql;

rc = sqlite3_open("test.db", &db);
if (rc != SQLITE_OK)
{
fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
exit(-1);
}
else
{
fprintf(stdout,"Open database successfully\n");
}

//sql language
sql = "create table company("\
"ID INT PRIMARY KEY NOT NULL,"\
"NAME TEXT NOT NULL,"\
"AGE INT NOT NULL,"\
"ADDRESS CHAR(50),"\
"SALARY REAL);";

rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
if (rc != SQLITE_OK)
{
fprintf(stderr, "SQL error: %s\n", zErrMsg);
sqlite3_free(zErrMsg);
}
else
{
fprintf(stdout, "Table created successfully\n");
}
sqlite3_close(db);
getchar();
return 0;

}
*/




//insert 操作
/*
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

static int callback(void *NotUsed, int argc, char** argv, char** azColName)
{
int i;
for (i = 0; i < argc; i++)
{
printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
}
printf("\n");
return 0;
}

int main(int argc, char* argv[])
{
sqlite3* db;
char* zErrMsg = 0;
int rc;
const char* sql;

rc = sqlite3_open("test.db", &db);
if (rc != SQLITE_OK)
{
fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
exit(-1);
}
else
{
fprintf(stdout, "Open database successfully\n");
}

sql = "INSERT INTO COMPANY(ID,NAME,AGE,ADDRESS,SALARY)"\
"VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \
"INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
"VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "     \
"INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
"VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );" \
"INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
"VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );";

rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
if (rc != SQLITE_OK)
{
fprintf(stderr, "SQL error: %s\n", zErrMsg);
sqlite3_free(zErrMsg);
}
else
{
fprintf(stdout, "Records create successfully\n");
}
sqlite3_close(db);
getchar();
return 0;
}
*/





//SELECT 操作
/*
typedef int(*sqlite3_callback)(
void*,    // Data provided in the 4th argument of sqlite3_exec()
int,      // The number of columns in row
char**,   // An array of strings representing fields in the row
char**    // An array of strings representing column names
);
*/
/*
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

static int callback(void *data, int argc, char* argv[], char** azColName)
{
int i;
fprintf(stdout, "%s\n", (const char*) data);
for (i = 0; i < argc; i++)
{
printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
}
printf("\n");
return 0;
}
int main(int argc, char* argv[])
{
sqlite3* db;
char* zErrMsg = 0;
int rc;
const char* sql;
const char* data = "Callback function called";

rc = sqlite3_open("test.db", &db);
if (rc != SQLITE_OK)
{
fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
exit(-1);
}
else
{
fprintf(stdout, "Open database successfully\n");
}

sql = "select * from company;";

rc = sqlite3_exec(db, sql, callback, (void *)data, &zErrMsg);
if (rc != SQLITE_OK)
{
fprintf(stderr, "SQL error: %s\n", zErrMsg);
sqlite3_free(zErrMsg);
}
else
{
fprintf(stdout, "Operation done successfully\n");
}
sqlite3_close(db);
getchar();
return 0;
}
*/



//UPDATE 操作
/*
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

static int callback(void *data, int argc, char **argv, char **azColName)
{
int i;
fprintf(stderr, "%s: ", (const char*)data);
for (i = 0; i<argc; i++)
{
printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
}
printf("\n");
return 0;
}

int main(int argc, char* argv[])
{
sqlite3 *db;
char *zErrMsg = 0;
int rc;
const char *sql;
const char* data = "Callback function called";

// Open database
rc = sqlite3_open("test.db", &db);
if (rc)
{
fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
exit(0);
}
else
{
fprintf(stderr, "Opened database successfully\n");
}

// Create merged SQL statement
sql = "UPDATE COMPANY set SALARY = 25000.00 where ID=1; " \
"SELECT * from COMPANY";

// Execute SQL statement
rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
if (rc != SQLITE_OK)
{
fprintf(stderr, "SQL error: %s\n", zErrMsg);
sqlite3_free(zErrMsg);
}
else
{
fprintf(stdout, "Operation done successfully\n");
}
sqlite3_close(db);
getchar();
return 0;
}
*/




//DELETE操作
/*
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

static int callback(void *data, int argc, char **argv, char **azColName)
{
	int i;
	fprintf(stderr, "%s: ", (const char*)data);
	for (i = 0; i<argc; i++)
	{
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

int main(int argc, char* argv[])
{
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	const char *sql;
	const char* data = "Callback function called";

	// Open database
	rc = sqlite3_open("test.db", &db);
	if (rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	}
	else
	{
		fprintf(stderr, "Opened database successfully\n");
	}

	// Create merged SQL statement
	sql = "DELETE from COMPANY where ID=2; " \
		"SELECT * from COMPANY";

	// Execute SQL statement
	rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
	if (rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	else
	{
		fprintf(stdout, "Operation done successfully\n");
	}
	sqlite3_close(db);
	getchar();
	return 0;
}
*/

