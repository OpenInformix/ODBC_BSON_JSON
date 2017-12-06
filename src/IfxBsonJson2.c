
/////////////////////////////////////////////////////////////
// Informix ODBC Applicatin Examples
// Copyright (c) 2017 OpenInformix. All rights reserved.
// Licensed under the Apache License, Version 2.0
//
// Authors:
//      Sathyanesh Krishnan


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define strdup _strdup
#endif

#ifdef DRIVER_MANAGER 
#include "sql.h"
#include "sqlext.h"
#else
#include <infxcli.h>
#endif



#define         MY_STR_LEN      (128 + 1)
#define         MY_REM_LEN      (254 + 1) 
#define         DATA_BUFF_SIZE  4096
unsigned char   DataBuffer1[DATA_BUFF_SIZE];
unsigned char   DataBuffer2[DATA_BUFF_SIZE];

typedef enum { BOTH, JSON, BSON } DataType;

void GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szMsgTag);
void ServerSetup(SQLHDBC hdbc, DataType SampleDataType);
int ReadResult(SQLHDBC hdbc, char *SqlSelect);
int PrintInfoSQLColumns(SQLHDBC hdbc, SQLCHAR *TableName);
void PrintBson(unsigned char *data, int len);
int ParamInsert(SQLHDBC hdbc, DataType SampleDataType);
const unsigned char *GetJsonData(unsigned n);


int main(int argc, char *argv[])
{
    SQLCHAR     ConnStrIn[1024] = "DSN=odbc1";
    SQLHANDLE   henv = NULL;
    SQLHANDLE   hdbc = NULL;
    int         rc = 0;

    char   *MyLocalConnStr = "DRIVER={IBM INFORMIX ODBC DRIVER};SERVER=srv1;DATABASE=xb1;HOST=xyz.abc.com;PROTOCOL=onsoctcp;SERVICE=5550;UID=user1;PWD=xyz;";

    if (argc == 1)
    {
        if (sizeof(int *) == 8)  // 64bit application 
        {
             MyLocalConnStr = "DRIVER={IBM INFORMIX ODBC DRIVER (64-bit)};SERVER = ids0; DATABASE = db1; HOST = 127.0.0.1; SERVICE = 9088; UID = informix; PWD = xyz; ";
        }
        strcpy((char *)ConnStrIn, MyLocalConnStr);

    }
    else if (argc == 2)
    {
        strcpy( (char *)ConnStrIn,  argv[1] );
    }
    else
    {
        strcpy((char *)ConnStrIn, MyLocalConnStr);

        if (0)
        {
            printf("\n Usage option is :");
            printf("\n %s    <Connection String>", argv[0]);
            printf("\n Example :");
            printf("\n %s   \"DSN=MyOdbcDsnName; uid=MyUserName; pwd=MyPassword;\" ", argv[0]);
            printf("\n OR ");
            printf("\n %s  \"%s\" ", argv[0], MyLocalConnStr);
            printf("\n\n");
            exit(0);
        }
    }


    rc = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &henv);

    rc = SQLSetEnvAttr(henv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);

    rc = SQLAllocHandle(SQL_HANDLE_DBC, henv, &hdbc);
    rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_ENV, henv, "SQLAllocHandle");

    printf("\n***************************************************\n");
    printf("\n Connecting with : \n [%s] \n", (char *)ConnStrIn);
    printf("\n***************************************************\n");

    rc = SQLDriverConnect(hdbc, NULL, ConnStrIn, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
    if (rc != 0)
    {
        printf("\n Connection Error (:- \n");
        GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLDriverConnect");
        goto Exit;
    }
    else
    {
        printf("\n Connection Success! \n");
    }

    if (1)
    {
        DataType SampleDataType = BOTH;

        SampleDataType = BSON; // only BSON
        SampleDataType = JSON; // only JSON
        //SampleDataType = BOTH; // both JSON and BSON

        ServerSetup(hdbc, SampleDataType);
        PrintInfoSQLColumns(hdbc, "t1");

        ParamInsert(hdbc, SampleDataType);

        ReadResult(hdbc, "SELECT * FROM t1");
    }


Exit:
    SQLDisconnect(hdbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hdbc);
    SQLFreeHandle(SQL_HANDLE_ENV, henv);

    return(0);
}


void GetDiagRec(SQLRETURN rc, SQLSMALLINT htype, SQLHANDLE hndl, char *szMsgTag)
{
    SQLCHAR message[SQL_MAX_MESSAGE_LENGTH + 1];
    SQLCHAR sqlstate[SQL_SQLSTATE_SIZE + 1];
    SQLINTEGER sqlcode = 0;
    SQLSMALLINT length = 0;

    if (szMsgTag == NULL)
    {
        szMsgTag = "---";
    }

    printf("\n %s: %d : ", szMsgTag, rc);
    if (rc >= 0)
    {
        printf(" OK [rc=%d] \n", rc);
    }
    else
    {
        int i = 1;
        printf(" FAILED : %i", rc);
        while (SQLGetDiagRec(htype,
            hndl,
            i,
            sqlstate,
            &sqlcode,
            message,
            SQL_MAX_MESSAGE_LENGTH + 1,
            &length) == SQL_SUCCESS)
        {
            printf("\n SQLSTATE          = %s", sqlstate);
            printf("\n Native Error Code = %ld", sqlcode);
            printf("\n %s", message);
            i++;
        }
        printf("\n-------------------------\n");
    }
}




void  ServerSetup(SQLHDBC hdbc, DataType SampleDataType)
{
    SQLRETURN   rc = 0;
    SQLHSTMT    hstmt = NULL;    
    int         i = 0;


    static unsigned char *SetupSqlsBoth[] =
    {
        "DROP TABLE t1;",
        "CREATE TABLE t1 (c1 int, c2 json, c3 bson)",
        "INSERT INTO  t1 VALUES (1, '{\"hello\": \"world\"}'::json, '{\"hello\": \"world\"}'::json::bson)",
        NULL,
    };

    static unsigned char *SetupSqlsJson[] =
    {
        "DROP TABLE t1;",
        "CREATE TABLE t1 (c1 int, c2 json)",
        //"CREATE TABLE t1 (c1 int, c2 lvarchar )",
        "INSERT INTO  t1 VALUES (1, '{\"hello\": \"world\"}'::json)",
        NULL,
    };

    static unsigned char *SetupSqlsBson[] =
    {
        "DROP TABLE t1;",
        "CREATE TABLE t1 (c1 int, c2 bson)",
        "INSERT INTO  t1 VALUES (1, '{\"hello\": \"world\"}'::json::bson)",
        NULL,
    };


    unsigned char **SetupSqls = SetupSqlsBoth;

    if (SampleDataType == BSON)
    {
        SetupSqls = SetupSqlsBson;
    }
    else if (SampleDataType == JSON)
    {
        SetupSqls = SetupSqlsJson;
    }

    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "ServerSetup::SQLAllocHandle::SQL_HANDLE_STMT");

    for (i = 0; SetupSqls[i] != NULL; ++i)
    {
        rc = SQLExecDirect(hstmt, SetupSqls[i], SQL_NTS);
        printf("\n[%d] %s", rc, SetupSqls[i]);
    }

    if (hstmt)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }
}


int ReadResult(SQLHDBC hdbc, char *SqlSelect)
{
    SQLRETURN       rc = 0;
    SQLHSTMT        hstmt = NULL;
    int             DataBufferSize = sizeof(DataBuffer1) - 2;

    printf("\n\n ----ReadResult ----");


    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLAllocHandle:SQL_HANDLE_STMT");


    rc = SQLExecDirect(hstmt, SqlSelect, SQL_NTS);
    if ((rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO))
    {
        SQLSMALLINT     ColumnCount = 0;
        int             RowNum = 0;


        rc = SQLNumResultCols(hstmt, &ColumnCount);
        printf("\n ----Number of colum in the result is %d\n", ColumnCount);

        while ((rc = SQLFetch(hstmt)) != SQL_NO_DATA)
        {
            SQLLEN StrLen_or_IndPtr = 0;
            int NumBytes = 0;
            int col = 0;

            ++RowNum;

            printf("\n{ --- Row# %d", RowNum);

            for (col = 1; col <= ColumnCount; ++col)
            {
                SQLLEN        cbNumericAttributePtr = 0;
                unsigned char MyCharacterAttributePtr[256];
                SQLSMALLINT   bufferLenUsed = 0;

                memset(DataBuffer1, 0, sizeof(DataBuffer1));
                StrLen_or_IndPtr = 0;

                rc = SQLGetData(hstmt, col, SQL_C_CHAR, DataBuffer1, DataBufferSize, &StrLen_or_IndPtr);
                if (rc == SQL_NO_DATA)
                {
                    break;
                }
                if (rc < 0)
                {
                    GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "SQLGetData");
                    break;
                }

                NumBytes = (int)((StrLen_or_IndPtr > DataBufferSize) || (StrLen_or_IndPtr == SQL_NO_TOTAL) ? DataBufferSize : StrLen_or_IndPtr);
                DataBuffer1[NumBytes] = 0;

                memset(MyCharacterAttributePtr, 0, sizeof(MyCharacterAttributePtr));
                rc = SQLColAttribute(hstmt, col, SQL_COLUMN_TYPE_NAME,
                            MyCharacterAttributePtr, (SQLSMALLINT)sizeof(MyCharacterAttributePtr), 
                            &bufferLenUsed, &cbNumericAttributePtr);
                printf("\n\n--Column Type : %s",  MyCharacterAttributePtr);

                
                if (strcmp("BSON", MyCharacterAttributePtr) == 0)
                {
                    // {"hello": "world"}
                    //            h  e  l  l  o            w  o  r  l  d
                    // 16 0 0 0 2 68 65 6C 6C 6F 0 6 0 0 0 77 6F 72 6C 64 0 0
                    PrintBson(DataBuffer1, (int)NumBytes);
                }
                else
                {
                    printf("\n %s", DataBuffer1);
                }
            }

            printf("\n}\n\n");
        }

    }
    else
    {
        GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, SqlSelect);
    }

    if (hstmt)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        rc = SQLFreeStmt(hstmt, SQL_UNBIND);
        rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }

    printf("\n");
    return (0);
}

void PrintBson(unsigned char *data, int len)
{
    int i = 0;
    unsigned char c = 0;

    printf("\n");

    for (i = 0; i < len; ++i)
    {
        c = *(data + i);
        // printf("%hhX ", c);
        //printf("0x%02x, ", c);
        printf("\\x%02x", c);

        if ((i > 0) && (i % 12 == 0))
        {
            // printf("\n");
        }

    }

}

int PrintInfoSQLColumns(SQLHDBC hdbc, SQLCHAR *TableName)
{
    SQLRETURN   rc = 0;
    SQLHSTMT    hstmt = NULL;
    int         ColNum = 0;

    // Declare buffers for result set data  
    SQLCHAR szCatalog[MY_STR_LEN];
    SQLCHAR szSchema[MY_STR_LEN];
    SQLCHAR szTableName[MY_STR_LEN];
    SQLCHAR szColumnName[MY_STR_LEN];
    SQLCHAR szTypeName[MY_STR_LEN];
    SQLCHAR szRemarks[MY_REM_LEN];
    SQLCHAR szColumnDefault[MY_STR_LEN];
    SQLCHAR szIsNullable[MY_STR_LEN];

    SQLINTEGER ColumnSize;
    SQLINTEGER BufferLength;
    SQLINTEGER CharOctetLength;
    SQLINTEGER OrdinalPosition;

    SQLSMALLINT DataType;
    SQLSMALLINT DecimalDigits;
    SQLSMALLINT NumPrecRadix;
    SQLSMALLINT Nullable;
    SQLSMALLINT SQLDataType;
    SQLSMALLINT DatetimeSubtypeCode;

    // Declare buffers for bytes available to return  
    SQLLEN cbCatalog;
    SQLLEN cbSchema;
    SQLLEN cbTableName;
    SQLLEN cbColumnName;
    SQLLEN cbDataType;
    SQLLEN cbTypeName;
    SQLLEN cbColumnSize;
    SQLLEN cbBufferLength;
    SQLLEN cbDecimalDigits;
    SQLLEN cbNumPrecRadix;
    SQLLEN cbNullable;
    SQLLEN cbRemarks;
    SQLLEN cbColumnDefault;
    SQLLEN cbSQLDataType;
    SQLLEN cbDatetimeSubtypeCode;
    SQLLEN cbCharOctetLength;
    SQLLEN cbOrdinalPosition;
    SQLLEN cbIsNullable;


    printf("\n\n ----PrintInfoSQLColumns ----");

    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLAllocHandle:SQL_HANDLE_STMT");

    /////////////////////////////////////////////////////////////////////////
    rc = SQLColumns(hstmt, NULL, 0, NULL, 0, TableName, SQL_NTS, NULL, 0);

    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) 
    {
        // Bind columns in result set to buffers  
        SQLBindCol(hstmt, 1, SQL_C_CHAR, szCatalog, MY_STR_LEN, &cbCatalog);
        SQLBindCol(hstmt, 2, SQL_C_CHAR, szSchema, MY_STR_LEN, &cbSchema);
        SQLBindCol(hstmt, 3, SQL_C_CHAR, szTableName, MY_STR_LEN, &cbTableName);
        SQLBindCol(hstmt, 4, SQL_C_CHAR, szColumnName, MY_STR_LEN, &cbColumnName);
        SQLBindCol(hstmt, 5, SQL_C_SSHORT, &DataType, 0, &cbDataType);
        SQLBindCol(hstmt, 6, SQL_C_CHAR, szTypeName, MY_STR_LEN, &cbTypeName);
        SQLBindCol(hstmt, 7, SQL_C_SLONG, &ColumnSize, 0, &cbColumnSize);
        SQLBindCol(hstmt, 8, SQL_C_SLONG, &BufferLength, 0, &cbBufferLength);
        SQLBindCol(hstmt, 9, SQL_C_SSHORT, &DecimalDigits, 0, &cbDecimalDigits);
        SQLBindCol(hstmt, 10, SQL_C_SSHORT, &NumPrecRadix, 0, &cbNumPrecRadix);
        SQLBindCol(hstmt, 11, SQL_C_SSHORT, &Nullable, 0, &cbNullable);
        SQLBindCol(hstmt, 12, SQL_C_CHAR, szRemarks, MY_REM_LEN, &cbRemarks);
        SQLBindCol(hstmt, 13, SQL_C_CHAR, szColumnDefault, MY_STR_LEN, &cbColumnDefault);
        SQLBindCol(hstmt, 14, SQL_C_SSHORT, &SQLDataType, 0, &cbSQLDataType);
        SQLBindCol(hstmt, 15, SQL_C_SSHORT, &DatetimeSubtypeCode, 0, &cbDatetimeSubtypeCode);
        SQLBindCol(hstmt, 16, SQL_C_SLONG, &CharOctetLength, 0, &cbCharOctetLength);
        SQLBindCol(hstmt, 17, SQL_C_SLONG, &OrdinalPosition, 0, &cbOrdinalPosition);
        SQLBindCol(hstmt, 18, SQL_C_CHAR, szIsNullable, MY_STR_LEN, &cbIsNullable);

        ColNum = 0;
        while (SQL_SUCCESS == rc)
        {
            szCatalog[0] = 0;
            szSchema[0] = 0;
            szTableName[0] = 0;
            szColumnName[0] = 0;
            DataType = 0;
            szTypeName[0] = 0;
            ColumnSize = 0;
            BufferLength = 0;
            DecimalDigits = 0;
            NumPrecRadix = 0;
            Nullable = 0;
            szRemarks[0] = 0;
            szColumnDefault[0] = 0;
            SQLDataType = 0;
            DatetimeSubtypeCode = 0;
            CharOctetLength = 0;
            OrdinalPosition = 0;
            szIsNullable[0] = 0;

            rc = SQLFetch(hstmt);
            
            if (rc == SQL_ERROR )
            {
                GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLFetch()");
            }

            if ( !(rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) )
            {
                break;
            }

            printf("\n\n ------ Col Number = %d -------", ++ColNum);
            
            // Catalog Name (database)
            printf("\n  1 TABLE_CAT szCatalog=[%s]", szCatalog);

            // name of the schema that contains TABLE_NAME
            printf("\n  2 TABLE_SCHEM szSchema=[%s]", szSchema);

            // name of the table, view, alias, or synonym.
            printf("\n  3 TABLE_NAME szTableName=[%s]", szTableName);

            // name of the column of the specified table, view, alias, or synonym.
            printf("\n  4 COLUMN_NAME szColumnName=[%s]", szColumnName);

            // Identifies the SQL data type of the column that COLUMN_NAME indicates.
            printf("\n  5 DATA_TYPE DataType=[%d]", DataType);

            // character string that represents the name of the data type that corresponds to the DATA_TYPE result set column
            printf("\n  6 TYPE_NAME szTypeName=[%s]", szTypeName);

            // If the DATA_TYPE column value denotes a character or binary string, 
            // this column contains the maximum length in characters for the column.
            printf("\n  7 COLUMN_SIZE ColumnSize=[%d]", ColumnSize);

            // maximum number of bytes for the associated C buffer to store data from  this column 
            // if SQL_C_DEFAULT is specified on the SQLBindCol(), SQLGetData(), and SQLBindParameter() calls. 
            // This length does not include any nul-terminator
            printf("\n  8 BUFFER_LENGTH BufferLength=[%d]", BufferLength);

            // scale of the column. NULL is returned for data types where scale is not applicable.
            printf("\n  9 DECIMAL_DIGITS DecimalDigits=[%d]", DecimalDigits);

            printf("\n 10 NUM_PREC_RADIX NumPrecRadix=[%d]", NumPrecRadix);

            // Contains SQL_NO_NULLS if the column does not accept null values.
            // Contains SQL_NULLABLE if the column accepts null values
            printf("\n 11 NULLABLE Nullable=[%d]", Nullable);

            //Contains any descriptive information about the column.
            printf("\n 12 REMARKS szRemarks=[%s]", szRemarks);

            // the default value for the column
            printf("\n 13 COLUMN_DEF szColumnDefault=[%s]", szColumnDefault);

            // Indicates the SQL data type. This column is the same as the DATA_TYPE column (#5)
            // SQL data types are the types in which data is stored in the data source.
            printf("\n 14 SQL_DATA_TYPE SQLDataType=[%d]", SQLDataType);

            // The subtype code for datetime data types (SQL_CODE_DATE, SQL_CODE_TIME, SQL_CODE_TIMESTAMP)
            printf("\n 15 SQL_DATETIME_SUB DatetimeSubtypeCode=[%d]", DatetimeSubtypeCode);

            // Contains the maximum length in bytes for a character data column. For single-byte character sets, this is the same as COLUMN_SIZE
            printf("\n 16 CHAR_OCTET_LENGTH CharOctetLength=[%d]", CharOctetLength);

            // The ordinal position of the column in the table. The first column in the table is number 1
            printf("\n 17 ORDINAL_POSITION OrdinalPosition=[%d]", OrdinalPosition);

            // Contains the string 'NO' if the column is known to be not nullable; and 'YES' otherwise.
            printf("\n 18 IS_NULLABLE szIsNullable=[%s]", szIsNullable);
        }
    }

    /////////////////////////////////////////////////////////////////////////
    if (hstmt)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        rc = SQLFreeStmt(hstmt, SQL_UNBIND);
        rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }

    return(rc);
}


const unsigned char *GetJsonData(unsigned n)
{
    static unsigned DocCount = 0;
    static unsigned char *JsonDataList[3];
    int i = 0;


    if (DocCount == 0)
    {
        memset(JsonDataList, 0, sizeof(JsonDataList));

        JsonDataList[DocCount++] = "{\"hello\": \"world 4\"}";
        JsonDataList[DocCount++] = "{\"Hello\": \"World 5\"}";
    }

    
    i = n%DocCount;

    return(JsonDataList[i]);
}

const unsigned char *GetBsonData(unsigned n)
{
    static unsigned DocCount = 0;
    static unsigned char *BsonDataList[3];
    static unsigned char bson1[] = { 0x18, 0x00, 0x00, 0x00, 0x02, 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x32, 0x00, 0x07,
                                                0x00, 0x00, 0x00, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x32, 0x00, 0x00 };
    int i = 0;


    if (DocCount == 0)
    {
        memset(BsonDataList, 0, sizeof(BsonDataList));

        BsonDataList[DocCount++] = bson1;
        BsonDataList[DocCount++] = (unsigned char *)"\x18\x00\x00\x00\x02\x68\x65\x6c\x6c\x6f\x32\x00\x07\x00\x00\x00\x77\x6f\x72\x6c\x64\x32\x00\x00";
    }


    i = n%DocCount;

    return(BsonDataList[i]);
}



int ParamInsert(SQLHDBC hdbc, DataType SampleDataType)
{
    SQLHSTMT            hstmt = NULL;
    SQLRETURN           rc = 0;
    int                 ErrorStatus = 0;
    char                *sql = NULL;
    SQLUSMALLINT        ParameterNumber = 0;
    unsigned int        RecCount = 0;
    
    SQLINTEGER c1 = 1002;
    SQLLEN     cbc1 = 0;
    SQLLEN     cbJson = 0;
    SQLLEN     cbBson = 0;


    printf("\n\n ----ParamInsert ----");

    rc = SQLAllocHandle(SQL_HANDLE_STMT, hdbc, &hstmt);
    rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_DBC, hdbc, "SQLAllocHandle:SQL_HANDLE_STMT");


    sql = "insert into t1 values ( ?, ?)";
    rc = SQLPrepare(hstmt, sql, SQL_NTS);

    ParameterNumber = 0;
    rc = SQLBindParameter(hstmt, ++ParameterNumber, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, SQL_NTS, 0, &c1, 0, &cbc1);

    // SQL Data Types
    // https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/sql-data-types
    // Converting Data from C to SQL Data Types
    // https://docs.microsoft.com/en-us/sql/odbc/reference/appendixes/converting-data-from-c-to-sql-data-types


    if (SampleDataType == JSON)
    {
        const int SQL_INFX_JSON = -115; // This will be replaced with #def in odbc header file 
        SQLSMALLINT MyCType = SQL_C_BINARY;  // if SQL_C_CHAR, then driver will do code set conversion.
        rc = SQLBindParameter( hstmt, ++ParameterNumber, SQL_PARAM_INPUT, MyCType, SQL_INFX_JSON, 
                                          DATA_BUFF_SIZE, 0, DataBuffer1, DATA_BUFF_SIZE, &cbJson);
    }
    else if (SampleDataType == BSON)
    {
        const int SQL_INFX_BSON = -116; // This will be replaced with #def in odbc header file 

        rc = SQLBindParameter(hstmt, ++ParameterNumber, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_INFX_BSON,
                                        DATA_BUFF_SIZE, 0, DataBuffer2, DATA_BUFF_SIZE, &cbBson);
    }

    
    c1 = 1000;
    for( RecCount = 0; RecCount<2; ++RecCount)
    {


        ///////////// C1 /////////////////
        ++c1;

        //////////// JSON ////////////////
        if (SampleDataType == JSON || SampleDataType == BOTH)
        {
            const unsigned char *pData = GetJsonData(RecCount);
            size_t DataLen = strlen(pData);

            memset(DataBuffer1, 0, sizeof(DataBuffer1));
            memcpy(DataBuffer1, pData, DataLen);
            cbJson = SQL_NTS;      // if, let driver to find the data size
            cbJson = DataLen;      // Let us tell driver about data size
        }

        //////////// BSON ////////////////
        if (SampleDataType == BSON || SampleDataType == BOTH)
        {
            const unsigned char *pData = GetBsonData(RecCount);
            unsigned int DataLen = *((unsigned short *)pData);


            memset(DataBuffer2, 0, sizeof(DataBuffer2));
            memcpy(DataBuffer2, pData, DataLen);
            cbBson = DataLen;      // Let us tell driver about data size
        }

        //////////// Execute /////////////
        rc = SQLExecute(hstmt);
        rc == 0 ? 0 : GetDiagRec(rc, SQL_HANDLE_STMT, hstmt, "SQLExecute");
    }


    if (hstmt)
    {
        SQLFreeStmt(hstmt, SQL_CLOSE);
        rc = SQLFreeStmt(hstmt, SQL_UNBIND);
        rc = SQLFreeStmt(hstmt, SQL_RESET_PARAMS);
        SQLFreeHandle(SQL_HANDLE_STMT, hstmt);
    }


    return(ErrorStatus);
}



