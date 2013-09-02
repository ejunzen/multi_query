#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <hiredis.h>
#include <mysql.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace std;

const static string redis_addr[] = {"10.4.21.229","xxx","xxx"};
const static string mysql_addr[] = {"10.4.21.229","xxx","xxx"};
const static int mysql_ports[] = {3306,3306,3306};
const static int redis_ports[] = {22120,22120,22120};
static string mysql_users[] = {"xxx","xxx","xxx"};
static string mysql_pwds[] = {"xxx","xxx","xxx"};
static string mysql_dbs[] = {"t2","t2","t2"};

namespace Query {

	class Query {
		public:
			virtual string query(string cmd)=0;
			virtual void setHost(string host)=0;
			virtual void setPort(int port)=0;
			virtual void setParameter(string user,string pwd, string db)=0;
	};

	class MysqlQuery: public Query {
		public:
			MysqlQuery(string host, int port){
				this->host_ = host;
				this->port_ = port;
			}
			~MysqlQuery(){
				mysql_close(& this->mysql_conn_);
			}
			string query(string cmd);
			void setHost(string host);
			void setPort(int port);
			void setParameter(string user,string pwd, string db);
		private:
			string host_;
			int port_;
			void getConnection();
			MYSQL mysql_conn_;
			string user_;
			string pwd_;
			string db_;
	};

	class RedisQuery: public Query {
		public:
			RedisQuery(string host,int port){
				this->host_ = host;
				this->port_ = port;
				getConnection();
			}
			~RedisQuery(){
				redisFree(this->con_);	
			}
			string query(string cmd);
			void setHost(string host);
			void setPort(int port);
			void setParameter(string user,string pwd, string db);
		private:
			string host_;
			int port_;
			void getConnection();
			redisContext *con_;
	};

	/**
	 * @brief create connection to redis
	 */
	void RedisQuery::getConnection(){
		redisContext *con = redisConnect(this->host_.c_str(), this->port_);
		if(con == NULL){
			fprintf(stderr,"error in connect to redis!");
			exit(103);
		}
		this->con_ = con;
	}

	/**
	 * @brief create connection to mysql
	 */
	void MysqlQuery::getConnection(){
		if(mysql_init(& this->mysql_conn_)==NULL){
			fprintf(stderr,"error happened in init mysql connection!");
			exit(101);
		}
		if(mysql_real_connect(& this->mysql_conn_, this->host_.c_str(), this->user_.c_str(), this->pwd_.c_str(), this->db_.c_str(), this->port_, NULL, 0)==NULL){
			fprintf(stderr,"error happened in connect to mysql!");
			exit(102);
		}
	}

	/**
	 * @brief set host to Redis
	 *
	 *
	 * @param host ip of the redis server
	 */
	void RedisQuery::setHost(string host){
		this->host_ = host;
		getConnection();
	}


	/**
	 * @brief set host of mysql
	 *
	 * @param host ip of the mysql server
	 */
	void MysqlQuery::setHost(string host){
		this->host_ = host;
		getConnection();
	}

	/**
	 * @brief set port of Redis
	 *
	 * @param port port of redis server
	 */
	void RedisQuery::setPort(int port){
		this->port_ = port;
		getConnection();
	}

	/**
	 * @brief set port of mysql
	 *
	 * @param port the port of mysql
	 */
	void MysqlQuery::setPort(int port){
		this->port_ = port;
		getConnection();
	}

	void MysqlQuery::setParameter(string user, string pwd, string db){
		this->user_ = user;
		this->pwd_ = pwd;
		this->db_ = db;
		getConnection();
	}

	void RedisQuery::setParameter(string user, string pwd, string db){
		return;
	}
	/**
	 * @brief excute the query
	 *
	 * @param cmd all cmd ,like keys *
	 *
	 * @return  null
	 */
	string RedisQuery::query(string cmd){
		redisReply *reply;
		reply = (redisReply*) redisCommand(this->con_, cmd.c_str());
		if(reply == NULL){
			fprintf(stderr,"error command!\n");
			exit(108);
		}
		if(reply->type != REDIS_REPLY_ARRAY){
			printf("RESULT: %s\n", reply->str);
			freeReplyObject(reply);
		}else{
			int j;
			for (j = 0; j < reply->elements; j++) {
				printf("%u) %s\n", j, reply->element[j]->str);
			}
			freeReplyObject(reply);
		}
		return "";
	}


	/**
	 * @brief print the result of the query
	 *
	 * @param cmd sql command
	 *
	 * @return null
	 */
	string MysqlQuery::query(string cmd){
		MYSQL_RES *mysql_result; /* Result handle */
		MYSQL_ROW mysql_row; /* Row data */
		int f1, f2, num_row, num_col;
		if (mysql_query(&this->mysql_conn_, cmd.c_str()) == 0) {	
			mysql_result = mysql_store_result(& this->mysql_conn_); // get the result from the executing select query
			num_row = mysql_num_rows(mysql_result); /* Get the no. of row */
			num_col = mysql_num_fields(mysql_result); /* Get the no. of column */

			for (f1 = 0; f1 < num_row; f1++) {
				mysql_row = mysql_fetch_row(mysql_result); /* Fetch one by one */
				for(f2 =0; f2 < num_col; f2++){
					printf("%s\t", mysql_row[f2]);
				}
				printf("\n");
			}
			mysql_free_result(mysql_result);
		}else{
			fprintf(stderr,"error in query,please check your sql!\n");
		}
		return "";
	}

}

int main(int argc, char* argv[]){

	char* cmd;
	char* mode;
	char* env;
	char ch;

	while ((ch = getopt(argc, argv, "vq:m:e:hs:")) != EOF) { 

		switch(ch){

			case 'v':
				printf("%s\n","v1.0,ejunzen@gmail.com");
				return 1;
			case 'h':
				printf("%s\n","usage:");
				printf("%s\n","\tquery -e [redis|mysql]-[local|test|online]");
				printf("%s\n","option:");
				printf("%s\n","\t-m [loop|not loop],default is not loop, just excute your cmd.");
				printf("%s\n","example:");
				printf("%s\n","\t query -q \"select * from message_info where msg_id = 1\" -e mysql-test -m loop");
				printf("%s\n","meanning:");
				printf("%s\n","\t it will query all tables which prefix is message_info int test enviroment!");
				return 2;           
			case 'm':
				mode = optarg;
				break;
			case 'e':
				env = optarg;
				break;
			default:
				break;
		}   
	}   	

	int e = -1;

	char rm[20]={0};
	char lto[20]= {0};

	for(int i =0;i < strlen(env);i++){
		if(env[i]=='-'){
			e = i;
			memcpy(rm,env,i);
			if(i< strlen(env)){
				memcpy(lto,env+i+1, strlen(env) -i);
			}	
			break;
		}
	}

	if(e < 0){
		fprintf(stderr,"please see the usage!\n");
		return 106;
	}

	int q = 0;
	if(strcmp(rm,"mysql")==0){
		q = 0;
	}else if(strcmp(rm,"redis")==0){
		q = 1;
	}else{
		fprintf(stderr,"unsuported query env, please select between mysql and redis!\n");
		return 107;	
	}

	if(strcmp(lto,"local")==0){
		e = 0;
	}else if(lto,"test"){
		e = 1;
	}else if(lto,"online"){
		e = 2;
	}else{
		fprintf(stderr,"unsuported query env, please select among local, test and online!\n");
		return 106;
	}

	/*
	   Query::Query *q = new Query::RedisQuery("localhost",63790);

	   q->query("keys *");

	   delete q;
	   */

	Query::Query *query;

	if(q==0){
		query = new Query::MysqlQuery(mysql_addr[e],mysql_ports[e]);
		query->setParameter(mysql_users[e],mysql_pwds[e],mysql_dbs[e]);
	}else{
		query = new Query::RedisQuery(redis_addr[e],redis_ports[e]);
	}

	string command;
	while(true){
		cout << "input command:" << endl;
		getline(cin,command);
		if(command == "exit"){
			fprintf(stdout,"thanks for using!\n");
			goto end;	
		}else if(command == "quit"){
			fprintf(stdout,"thanks for using!\n");
			goto end;	
		}
		query->query(command);
	}
	delete query;
end:
	return 0;
}
