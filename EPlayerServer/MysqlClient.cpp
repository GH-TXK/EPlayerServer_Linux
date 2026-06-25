#include "MysqlClient.h"
#include <sstream>
#include "Logger.h"
#include <iostream> 

int CMysqlClient::Connect(const KeyValue& args)
{
	if (m_bInit)return -1;
	MYSQL* ret = mysql_init(&m_db);
	if (ret == NULL)return -2;
	ret = mysql_real_connect(&m_db,
		args.at("host"), args.at("user"),
		args.at("password"), args.at("db"),
		atoi(args.at("port")),
		NULL, 0);
	if ((ret == NULL) && (mysql_errno(&m_db) != 0)) {
		printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("%d %s", mysql_errno(&m_db), mysql_error(&m_db));
		mysql_close(&m_db);
		bzero(&m_db, sizeof(m_db));
		return -3;
	}
	m_bInit = true;
	return 0;
}

int CMysqlClient::Exec(const Buffer& sql)
{
	if (!m_bInit)return -1;
	int ret = mysql_real_query(&m_db, sql, sql.size());
	if (ret != 0) {
		printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("%d %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}
	return 0;
}

//int CMysqlClient::Exec(const Buffer& sql, Result& result, const _Table_& table)
//{
//	if (!m_bInit) return -1;
//
//	int ret = mysql_real_query(&m_db, sql.data(), sql.size());
//	if (ret != 0) {
//		printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
//		TRACEE("%d %s", mysql_errno(&m_db), mysql_error(&m_db));
//		return -2;
//	}
//
//	MYSQL_RES* res = mysql_store_result(&m_db);
//	if (!res) {
//		if (mysql_field_count(&m_db) == 0) return 0; // 非查询语句
//		printf("%s store_result error: %s\n", __FUNCTION__, mysql_error(&m_db));
//		return -3;
//	}
//
//	// RAII 确保释放
//	auto freeGuard = [](MYSQL_RES* r) { mysql_free_result(r); };
//	std::unique_ptr<MYSQL_RES, decltype(freeGuard)> guard(res, freeGuard);
//
//	unsigned num_fields = mysql_num_fields(res);
//	MYSQL_ROW row;
//	while ((row = mysql_fetch_row(res)) != nullptr) {
//		PTable pt = table.Copy();
//		unsigned safe_count = std::min(num_fields, (unsigned)pt->FieldDefine.size());
//		for (unsigned i = 0; i < safe_count; i++) {
//			if (row[i] != nullptr) {
//				pt->FieldDefine[i]->LoadFromStr(row[i]);
//			}
//		}
//		result.push_back(std::move(pt)); // 避免不必要的拷贝
//	}
//	return 0;
//}

int CMysqlClient::Exec(const Buffer& sql, Result& result, const _Table_& table)
{
	if (!m_bInit)return -1;
	int ret = mysql_real_query(&m_db, sql, sql.size());
	if (ret != 0) {
		printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("%d %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}
	MYSQL_RES* res = mysql_store_result(&m_db);
	MYSQL_FIELD* fieldsName = mysql_fetch_fields(res);
	MYSQL_ROW row;
	unsigned num_fields = mysql_num_fields(res);
	while ((row = mysql_fetch_row(res)) != NULL) {
		PTable pt = table.Copy();
		for (unsigned i = 0; i < num_fields; i++) {
			if (row[i] != NULL) {
				pt->FieldDefine[i]->LoadFromStr(row[i]);
			}
		}
		result.push_back(pt);
		 //====== 新增：结果集全面体检日志 ======
		if (!result.empty()) {
			auto user1 = pt;
			TRACEI("===== 数据库结果集体检开始 =====");
			TRACEI("结果集总行数: %d", (int)result.size());

			//遍历 Fields 这个 map，看看里面到底装了什么
			for (auto& pair : user1->Fields) {
				PField field = pair.second;
				if (field != nullptr) {
					// 打印：Map的Key、对象内部的Name、nType、以及Value.String的内容
					TRACEI("字段 -> MapKey=[%s], Name=[%s], nType=%d, StringPtr=%s",
						(char*)pair.first,
						(char*)field->Name,
						field->nType,
						(char*)field->toEqualExp());
				}
				else {
					TRACEE("字段 -> MapKey=[%s], 但 PField 指针本身是 nullptr!", (char*)pair.first);
				}
			}
			TRACEI("===== 数据库结果集体检结束 =====");
		}
		else {
			TRACEE("数据库查询结果为空！");
		}
		 //================================
	}
	return 0;
}
int CMysqlClient::StartTransaction()
{
	if (!m_bInit)return -1;
	int ret = mysql_real_query(&m_db, "BEGIN", 6);
	if (ret != 0) {
		printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("%d %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}
	return 0;
}

int CMysqlClient::CommitTransaction()
{
	if (!m_bInit)return -1;
	int ret = mysql_real_query(&m_db, "COMMIT", 7);
	if (ret != 0) {
		printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("%d %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}
	return 0;
}

int CMysqlClient::RollbackTransaction()
{
	if (!m_bInit)return -1;
	int ret = mysql_real_query(&m_db, "ROLLBACK", 9);
	if (ret != 0) {
		printf("%s %d %s\n", __FUNCTION__, mysql_errno(&m_db), mysql_error(&m_db));
		TRACEE("%d %s", mysql_errno(&m_db), mysql_error(&m_db));
		return -2;
	}
	return 0;
}

int CMysqlClient::Close()
{
	if (m_bInit) {
		m_bInit = false;
		mysql_close(&m_db);
		bzero(&m_db, sizeof(m_db));
	}
	return 0;
}

bool CMysqlClient::IsConnected()
{
	return m_bInit;
}

_mysql_table_::_mysql_table_(const _mysql_table_& table)
{
	Database = table.Database;
	Name = table.Name;
	for (size_t i = 0; i < table.FieldDefine.size(); i++)
	{
		PField field = PField(new _mysql_field_(*
			(_mysql_field_*)table.FieldDefine[i].get()));
		FieldDefine.push_back(field);
		Fields[field->Name] = field;
	}
}

_mysql_table_::~_mysql_table_()
{
}

Buffer _mysql_table_::Create()
{
	//CREATE TABLE IF NOT EXISTS 表全名 (列定义,..., PRIMARY KEY `主键列名` ,UNIQUE INDEX `列名_UNIQUE` (列名 ASC) VISIBLE );
	Buffer sql = "CREATE TABLE IF NOT EXISTS " + (Buffer)*this + " (\r\n";
	for (unsigned i = 0; i < FieldDefine.size(); i++)
	{
		if (i > 0)sql += ",\r\n";
		sql += FieldDefine[i]->Create();
		if (FieldDefine[i]->Attr & PRIMARY_KEY) {
			sql += ",\r\n PRIMARY KEY (`" + FieldDefine[i]->Name + "`)";
		}
		if (FieldDefine[i]->Attr & UNIQUE) {
			sql += ",\r\n UNIQUE INDEX `" + FieldDefine[i]->Name + "_UNIQUE` (";
			sql += (Buffer)*FieldDefine[i] + " ASC) VISIBLE ";
		}
	}
	sql += ");";
	printf("sql = %s\n", (char*)sql);
	return sql;
}

Buffer _mysql_table_::Drop()
{
	return "DROP TABLE" + (Buffer)*this;
}

Buffer _mysql_table_::Insert(const _Table_& values)
{
	// INSERT INTO 表全名 (列名,...)VALUES(值,...);
	Buffer sql = "INSERT INTO " + (Buffer)*this + " (";
	bool isfirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_INSERT) {
			if (!isfirst)sql += ",";
			else isfirst = false;
			sql += (Buffer)*values.FieldDefine[i];
		}
	}
	sql += ") VALUES (";
	isfirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_INSERT) {
			if (!isfirst)sql += ",";
			else isfirst = false;
			sql += values.FieldDefine[i]->toSqlStr();
		}
	}
	sql += " );";
	printf("sql = %s\n", (char*)sql);
	return sql;
}

Buffer _mysql_table_::Delete(const _Table_& values)
{
	Buffer sql = "DELETE FROM " + (Buffer)*this + " ";
	Buffer Where = "";
	bool isfirst = true;
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		if (FieldDefine[i]->Condition & SQL_CONDITION) {
			if (!isfirst)Where += " AND ";
			else isfirst = false;
			Where += (Buffer)*FieldDefine[i] + "=" + FieldDefine[i]->toSqlStr();
		}
	}
	if (Where.size() > 0)
		sql += " WHERE " + Where;
	sql += ";";
	printf("sql = %s\r\n", (char*)sql);
	return sql;
}

Buffer _mysql_table_::Modify(const _Table_& values)
{
	Buffer sql = "UPDATE " + (Buffer)*this + " SET ";
	bool isfirst = true;
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_MODIFY) {
			if (!isfirst)sql += ",";
			else isfirst = false;
			sql += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlStr();
		}
	}

	Buffer Where = "";
	for (size_t i = 0; i < values.FieldDefine.size(); i++) {
		if (values.FieldDefine[i]->Condition & SQL_CONDITION) {
			if (!isfirst)Where += " AND ";
			else isfirst = false;
			Where += (Buffer)*values.FieldDefine[i] + "=" + values.FieldDefine[i]->toSqlStr();
		}
	}
	if (Where.size() > 0)
		sql += " WHERE " + Where;
	sql += " ;";
	printf("sql = %s\n", (char*)sql);
	return sql;
}

Buffer _mysql_table_::Query(const Buffer& condition)
{
	Buffer sql = "SELECT ";
	for (size_t i = 0; i < FieldDefine.size(); i++)
	{
		if (i > 0)sql += ',';
		sql += '`' + FieldDefine[i]->Name + "` ";
	}
	sql += " FROM " + (Buffer)*this + " ";
	if (condition.size() > 0) {
		sql += " WHERE " + condition;
	}
	sql += ";";
	std::cout << "Generated SQL: " << sql << std::endl;
	printf("sql = %s\n", (char*)sql);
	TRACEI("sql = %s\n", (char*)sql);
	return sql;
}

PTable _mysql_table_::Copy() const
{
	return PTable(new _mysql_table_(*this));
}

void _mysql_table_::ClearFieldUsed()
{
	for (size_t i = 0; i < FieldDefine.size(); i++) {
		FieldDefine[i]->Condition = 0;
	}
}

_mysql_table_::operator const Buffer() const
{
	Buffer Head;
	if (Database.size())
		Head = '`' + Database + "`.";
	return Head + '`' + Name + '`';
}

_mysql_field_::_mysql_field_() :_Field_()
{
	nType = TYPE_NULL;
	Value.String = nullptr; // 【关键修复】：统一将指针初始化为空，防止野指针
}

_mysql_field_::_mysql_field_(int ntype, const Buffer& name, unsigned attr, const Buffer& type, const Buffer& size, const Buffer& default_, const Buffer& check)
{
	nType = ntype;
	switch (ntype)
	{
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		Value.String = new Buffer();
		break;
	}

	Name = name;
	Attr = attr;
	Type = type;
	Size = size;
	Default = default_;
	Check = check;
}

_mysql_field_::_mysql_field_(const _mysql_field_& field)
{
	TRACEI("父类nType地址: %p", &(this->_Field_::nType));
	TRACEI("当前nType地址: %p", &(this->nType));
	nType = field.nType;
	Value.String = nullptr; // 【关键修复】：先初始化为空

	switch (field.nType)
	{
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		// 【关键修复】：必须检查原对象的指针是否为空，才能进行拷贝！
		if (field.Value.String != nullptr) {
			Value.String = new Buffer(*field.Value.String);
		}
		else {
			Value.String = nullptr;
		}
		break;
	default:
		// 如果不是字符串类型，直接拷贝 union 里的其他值
		Value.Integer = field.Value.Integer;
		break;
	}

	Name = field.Name;
	Attr = field.Attr;
	Type = field.Type;
	Size = field.Size;
	Default = field.Default;
	Check = field.Check;
}

_mysql_field_::~_mysql_field_()
{
	switch (nType)
	{
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		if (Value.String) {
			Buffer* p = Value.String;
			Value.String = NULL;
			delete p;
		}
		break;
	}
}

Buffer _mysql_field_::Create()
{
	Buffer sql = "`" + Name + "` " + Type + Size + " ";
	if (Attr & NOT_NULL) {
		sql += "NOT NULL";
	}
	else {
		sql += "NULL";
	}
	//BLOB TEXT GEOMETRY JSON不能有默认值的
	if ((Attr & DEFAULT) && (Default.size() > 0) && (Type != "BLOB") && (Type != "TEXT") && (Type != "GEOMETRY") && (Type != "JSON"))
	{
		// 如果是时间戳关键字，直接拼接，不加引号
		if (Default == "CURRENT_TIMESTAMP") {
			sql += " DEFAULT CURRENT_TIMESTAMP ";
		}
		// 其他普通的默认值，保留原有的双引号包裹
		else {
			sql += " DEFAULT \"" + Default + "\" ";
		}
	}
	//UNIQUE PRIMARY_KEY 外面处理
	//CHECK mysql不支持
	if (Attr & AUTOINCREMENT) {
		sql += " AUTO_INCREMENT ";
	}
	return sql;
}

void _mysql_field_::LoadFromStr(const Buffer& str)
{
	TRACEI("映射字段: Name=[%s], nType=%d, 数据=[%s]", (char*)Name, nType, (char*)str);
	switch (nType)
	{
	case TYPE_NULL:
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		Value.Integer = atoi(str);
		break;
	case TYPE_REAL:
		Value.Double = atof(str);
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
		*Value.String = str;
		break;
	case TYPE_BLOB:
		*Value.String = Str2Hex(str);
		break;
	default:
		printf("type=%d\n", nType);
		break;
	}
	// 假设 field 是一个 _mysql_field_ 对象或指针
	switch (nType) {
	case TYPE_BOOL:
		TRACEI("Value.Bool = %d\n", Value.Bool);
		break;
	case TYPE_INT:
	case TYPE_DATETIME:
		TRACEI("Value.Integer = %d\n", Value.Integer);
		break;
	case TYPE_REAL:
		TRACEI("Value.Double = %f\n", Value.Double);
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		// 【注意】：Buffer* 是指针，必须判空后再解引用打印！
		if (Value.String != nullptr) {
			TRACEI("Value.String = %s\n", (char*)(*Value.String));
		}
		else {
			TRACEI("Value.String = (nil)\n");
		}
		break;
	default:
		TRACEI("Unknown type\n");
		break;
	}
}

Buffer _mysql_field_::toEqualExp() const
{
	Buffer sql = (Buffer)*this + " = ";
	std::stringstream ss;
	switch (nType)
	{
	case TYPE_NULL:
		sql += " NULL ";
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		ss << Value.Integer;
		sql += ss.str() + " ";
		break;
	case TYPE_REAL:
		ss << Value.Double;
		sql += ss.str() + " ";
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		sql += '"' + *Value.String + "\" ";
		break;
	default:
		printf("type=%d\n", nType);
		break;
	}
	return sql;
}

Buffer _mysql_field_::toSqlStr() const
{
	Buffer sql = "";
	std::stringstream ss;
	switch (nType)
	{
	case TYPE_NULL:
		sql += " NULL ";
		break;
	case TYPE_BOOL:
	case TYPE_INT:
	case TYPE_DATETIME:
		ss << Value.Integer;
		sql += ss.str() + " ";
		break;
	case TYPE_REAL:
		ss << Value.Double;
		sql += ss.str() + " ";
		break;
	case TYPE_VARCHAR:
	case TYPE_TEXT:
	case TYPE_BLOB:
		sql += '"' + *Value.String + "\" ";
		break;
	default:
		printf("type=%d\n", nType);
		break;
	}
	return sql;
}

_mysql_field_::operator const Buffer() const
{
	return '`' + Name + '`';
}

Buffer _mysql_field_::Str2Hex(const Buffer& data) const
{
	const char* hex = "0123456789ABCDEF";
	std::stringstream ss;
	for (auto ch : data)
		ss << hex[(unsigned char)ch >> 4] << hex[(unsigned char)ch & 0xF];
	return ss.str();
}
