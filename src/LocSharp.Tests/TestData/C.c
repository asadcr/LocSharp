/* implement the main bcon interface */

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <dirent.h>
#include <libgen.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>

#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <assert.h>
#include <syslog.h>

#include <libxml/parser.h>

#include "bcon.h"
#include "bcon_err.h"
#include "bcon_xml.h"
#include "bvsp_internal.h"	/* stolen BVSP_ERR_* symbols */
#include "hash.h"



static int	  bcon_initialized = BVSP_ERR_NOT_INITIALIZED;
#define BCON_INITIALIZATION_FAILED -BVSP_ERR_NOT_INITIALIZED

#define LOCK_TIMEOUT_SECONDS    2
static const char *lock_prefix = ".lock";

#define BCON_TRIGGER_PATH "/bivio/bcon_triggers/bcon_trigger/" 
#define BCON_TRIGGER_PATH_WITH_NAME "/bivio/bcon_triggers/bcon_trigger[@name=\"" 
#define BCON_TRIGGER_PATH_END_NAME "\"]/" 
#define BCON_TRIGGER_PATH_END_NAME_WITH_PRE_ASTRK "\"]/pre_triggers/*" 
#define BCON_TRIGGER_PATH_END_NAME_WITH_PRE_NO_ASTRK "\"]/pre_triggers/" 
#define BCON_TRIGGER_PATH_END_NAME_WITH_POST_ASTRK "\"]/post_triggers/*" 
#define BCON_TRIGGER_PATH_END_NAME_WITH_POST_NO_ASTRK "\"]/post_triggers/" 
#define BCON_TRIGGER_PATH_END_NAME_WITH_GET_ASTRK "\"]/get_triggers/*" 
#define BCON_TRIGGER_PATH_END_NAME_WITH_GET_NO_ASTRK "\"]/get_triggers/" 
#define BCON_TRIGGER_PATH_SET "set["
#define BCON_TRIGGER_PATH_CREATE "create["
#define BCON_TRIGGER_PATH_UNSET "unset["
#define BCON_TRIGGER_PATH_GET "get["
#define BCON_TRIGGER_PATH_ATTR_KEY "]/@key"
#define BCON_TRIGGER_PATH_ATTR_EXEC "]/@exec"
static const char *BCON_TRIGGER_PATH_TRIGGER_TYPES[] = { BCON_TRIGGER_PATH_SET,
						  	 BCON_TRIGGER_PATH_CREATE,
						   	 BCON_TRIGGER_PATH_UNSET,
							 BCON_TRIGGER_PATH_GET };

#define TRIGGER_TYPE_SET 0
#define TRIGGER_TYPE_CREATE 1
#define TRIGGER_TYPE_UNSET 2
#define TRIGGER_TYPE_GET 3

#define TRIGGER_PRE 1
#define TRIGGER_POST 2
#define TRIGGER_GET 3

#define SAVE_WITH_TRIGGER 0
#define SAVE_WITHOUT_TRIGGER 1
#define TRIGGER_WITHOUT_SAVE 2


static const char *TRIGGER_TYPE_STR[] = { "set", "create", "delete", "get" };
static const char *TRIGGER_OP_STR[] = { "unused", "pre", "post", "get" };

typedef struct bcon_config_type {
        int 	type; 
        char 	*db_path;
        char 	*trigger_db_path;
	time_t 	db_mtime;
	time_t 	trigger_db_mtime;
	BConCfgPtr db_cfg;
	BConCfgPtr trigger_db_cfg;
        void 	*transaction;
	void	*lock;
	int	lock_fd;
	int handle;
	char	*last_error;
	int	last_error_code;
	int	last_output_error_code;
	char	*last_output_buffer;
	char	*last_output_buffer_error;
	void	*parent_key;
	void	*child_nodes;
	hash_tbl *tr_hash_table;
	char	**trans_data;
	int	trans_count;
        union {
        } handle_union;
} BCON_CONFIG;

#define TRANS_TYPE_SET 0
#define TRANS_TYPE_UNSET 1


typedef struct trans_type {
	void	*hash_key;
	void	*node_key;
	void	*remove_key;
	int	trans_action;
	void	*data;
} TRANSACTION_TYPE;

typedef struct trigger_action_type {
	char exec_key[2048];
	char exec_cmd[2048];
} TRIGGER_ACTION;

typedef struct trigger_match_type {
	char trigger_path[2048];
	char *match_key;
	int op;
	int type;
	int numb_of_key_values;
	TRIGGER_KEY_VALUE **key_values;
	int numb_of_actions;
	TRIGGER_ACTION **actions;
} TRIGGER_MATCH;

static int next_handle = 0;
static int default_bcon_handle = 0;
static BCON_CONFIG *default_bcon_config = NULL;
static int next_trans = 0;

#define DB_STORE_TYPE 0
#define DB_TRIGGER_TYPE 1 

typedef int boolean;
#define INIT_HASH_TABLE_SIZE 8
static hash_tbl *db_hash_table = NULL;

int *bcon_set_multiple_db_va_list(int bcon_handle, int num, va_list ap);
BCON_CONFIG *bcon_get_config(int bcon_handle);
int bcon_set_by_action(int action, const char *key, const void *val, int len);
int bcon_set_db_by_action(int bcon_handle, int action, const char *key, const void *val, int len);
int *bcon_set_multiple_by_action(int action, const char **key, const char **val, int *len, int num);
int *bcon_set_multiple_db_by_action(int bcon_handle, int action, const char **key, const char **val, int *len, int num);
int *bcon_set_multiple_va_list_by_action(int action, int num, va_list ap);
int *bcon_set_multiple_db_va_list_by_action(int bcon_handle, int action, int num, va_list ap);
int bcon_unset_by_action(int action, const char *key);
int bcon_unset_db_by_action(int bcon_handle, int action, const char *key);
int bcon_set_node_from_copy_db(BConCfgPtr db_cfg, void *node_copy, const char *parent_key);
void bcon_node_free(void *node_copy);
int bcon_trans_commit_db(int bcon_handle);
int bcon_trans_revert_db(int bcon_handle);
int bcon_trans_end_db(int bcon_handle);
int bcon_trans_start_db(int bcon_handle);



typedef struct ErrorConstant ErrorConstant;
struct ErrorConstant
{
        int		value;          /* value represented by the name */
        const char	*description;   /* human readable description */
};

static const ErrorConstant bcon_errors[] = {
  { BCON_INITIALIZATION_FAILED,"Configuration initialization failed" },
  { BVSP_ERR_SUCCESS,       "Success" },
  { BVSP_ERR_NO_MEMORY,     "Out of memory" },
  { BVSP_ERR_TIMEOUT,       "Timeout" },
  { BVSP_ERR_INTERNAL_FAILURE,"Internal Failure" },
  { BVSP_ERR_OPEN_FAILED,   "Could not open file" },
  { BVSP_ERR_HOST_NOT_FOUND,"Host lookup failed" },
  { BVSP_ERR_SOCKET_FAILED, "Could not open socket" },
  { BVSP_ERR_CONNECT_FAILED,"Connection Failed" },
  { BVSP_ERR_IOCTL_FAILED,  "ioctl() failed" },
  { BVSP_ERR_SELECT_FAILED, "Error from select()" },
  { BVSP_ERR_WRITE_FAILED,  "Write failed" },
  { BVSP_ERR_READ_FAILED,   "Read failed" },
  { BVSP_ERR_NO_DATA,       "Nothing received" },
  { BVSP_ERR_DECODE_FAILED, "Could not decode message" },
  { BVSP_ERR_CONFIG_FAILED, "Config info not found" },
  { BVSP_ERR_INVALID_ARG,   "Invalid Argument" },
  { BVSP_ERR_DUPLICATE,     "Duplicate Entry" },
  { BVSP_ERR_NOT_FOUND,     "Not Found" },
  { BVSP_ERR_AMBIGUOUS,     "Ambiguous" },
  { BVSP_ERR_MISSING_ARG,   "Expected argument missing" },
  { BVSP_ERR_NO_PEER,       "No peer is present" },
  { BVSP_ERR_BAD_TYPE,      "Excepted type not received" },
  { BVSP_ERR_TRUNCATED,     "Returned data truncated" },
  { BVSP_ERR_FAILED,        "Operation failed" },
  { BVSP_ERR_NOT_INITIALIZED,"Configuration not initialized" },
  { BVSP_ERR_FAILED,        "Operation failed" },
  { BVSP_ERR_NO_ROOM,       "No more room in table" },
  { BVSP_ERR_HMDB_FAILED,   "HMDB operation failed" },
  { 0, NULL }
};

static const char *Default_Xml_Config = {
"<?xml version=\"1.0\"?>\n" \
"<bivio>\n" \
"  <bvcig>\n" \
"    <settings>\n" \
"    </settings>\n" \
"    <paths>\n" \
"      <def_conf_file>/etc/bivio/bvcig/bvcig.xml</def_conf_file>\n" \
"      <last_conf_file>/etc/bivio/bvcig/bvcig-last.xml</last_conf_file>\n" \
"      <log_file>/var/log/bivio/bvcig.cf</log_file>\n" \
"      <tctemp_file>/var/tmp/tctemp/test.tc</tctemp_file>\n" \
"      <dbdir>/var/lib/bivio/bvcig/db</dbdir>\n" \
"      <db_info_file>/etc/bivio/bvcig/rc.bvcigdb</db_info_file>\n" \
"      <lib_log_file>/var/log/bivio/bvcig-lib.cf</lib_log_file>\n" \
"      <tctemp_dir>/var/tmp/tctemp</tctemp_dir>\n" \
"      <scripts>\n" \
"        <dir>/var/lib/bivio/bvcig/scripts</dir>\n" \
"        <updated.d>/var/lib/bivio/bvcig/scripts/updated.d</updated.d>\n" \
"        <action_alias>/var/lib/bivio/bvcig/scripts/action_alias</action_alias>\n" \
"        <action_check>/var/lib/bivio/bvcig/scripts/action_check</action_check>\n" \
"        <ext_action_check>/var/lib/bivio/bvcig/scripts/ext_action_check</ext_action_check>\n" \
"        <class_alias>/var/lib/bivio/bvcig/scripts/class_alias</class_alias>\n" \
"        <class_check>/var/lib/bivio/bvcig/scripts/class_check</class_check>\n" \
"        <ext_class_check>/var/lib/bivio/bvcig/scripts/ext_class_check</ext_class_check>\n" \
"      </scripts>\n" \
"      <config>/etc/bivio/bvcig</config>\n" \
"      <examples>/etc/bivio/bvcig/examples</examples>\n" \
"    </paths>\n" \
"  </bvcig>\n" \
"  <bvsep>\n" \
"    <paths>\n" \
"      <root>/var/lib/bivio/bvsep</root>\n" \
"      <prog_script>/var/lib/bivio/bvsep/prog_sep</prog_script>\n" \
"    </paths>\n" \
"  </bvsep>\n" \
"  <blm>\n" \
"    <paths>\n" \
"      <modules>/usr/lib/bivio/blm/modules</modules>\n" \
"      <db>/var/lib/bivio/blm/blmdb</db>\n" \
"      <private_key>/var/lib/bivio/blm/blm-key.pem</private_key>\n" \
"      <log>/var/log/bivio/blm.cf</log>\n" \
"    </paths>\n" \
"  </blm>\n" \
"  <nrsp>\n" \
"    <ioc>\n" \
"      <paths>\n" \
"        <defaults>/etc/bivio/nrsp/iocfg/defaults</defaults>\n" \
"        <scripts>\n" \
"          <slotup>/etc/bivio/nrsp/scripts/slotup</slotup>\n" \
"          <slotdown>/etc/bivio/nrsp/scripts/slotdown</slotdown>\n" \
"          <dir>/etc/bivio/nrsp/scripts</dir>\n" \
"        </scripts>\n" \
"        <config>/etc/bivio/nrsp/iocfg</config>\n" \
"      </paths>\n" \
"    </ioc>\n" \
"    <carddb>\n" \
"      <db>\n" \
"        <pass>NrSp1234</pass>\n" \
"        <conn_timeout>3</conn_timeout>\n" \
"        <pkt_timeout>5</pkt_timeout>\n" \
"        <name>carddb</name>\n" \
"        <port>7685</port>\n" \
"        <login>CarddbAdmin</login>\n" \
"        <host>CPU-X</host>\n" \
"      </db>\n" \
"      <system_type></system_type>\n" \
"      <RESET>1</RESET>\n" \
"      <VR-MODE></VR-MODE>\n" \
"      <MFG></MFG>\n" \
"      <paths>\n" \
"        <hosts>/etc/hosts</hosts>\n" \
"        <dbfile>/var/lib/bivio/nrsp/db/carddb</dbfile>\n" \
"        <local_hosts>/etc/hosts</local_hosts>\n" \
"        <db_log>/var/log/bivio/carddb-dblog.cf</db_log>\n" \
"        <hosts_xpc>/etc/hosts.xpc</hosts_xpc>\n" \
"        <boot_dir>/boot</boot_dir>\n" \
"        <cpu_skel>/bivio/shared/cpus.skel</cpu_skel>\n" \
"        <cpu_dirs>/bivio/shared/cpus</cpu_dirs>\n" \
"        <dbdir>/var/lib/bivio/nrsp/db</dbdir>\n" \
"        <scripts>\n" \
"          <dbpull>/etc/bivio/nrsp/scripts/dbpull</dbpull>\n" \
"          <new_vslots>/etc/bivio/nrsp/scripts/new_vslots</new_vslots>\n" \
"          <dir>/etc/bivio/nrsp/scripts</dir>\n" \
"        </scripts>\n" \
"        <hosts_base>/etc/hosts_base</hosts_base>\n" \
"        <action_log>/var/log/bivio/carddb-actions.cf</action_log>\n" \
"        <logfile>/var/log/bivio/nrboard.cf</logfile>\n" \
"      </paths>\n" \
"    </carddb>\n" \
"    <hm>\n" \
"      <paths>\n" \
"        <fault_log>/var/log/bivio/hm-faults.cf</fault_log>\n" \
"        <scripts>\n" \
"          <dir>/etc/bivio/nrsp/scripts/hm</dir>\n" \
"        </scripts>\n" \
"      </paths>\n" \
"    </hm>\n" \
"    <settings>\n" \
"      <stack_ifname>nr0</stack_ifname>\n" \
"      <act_only_alarms>0</act_only_alarms>\n" \
"      <auto_failback>1</auto_failback>\n" \
"      <active_standby>1</active_standby>\n" \
"      <start_nrvrrpd>1</start_nrvrrpd>\n" \
"    </settings>\n" \
"    <scsi>\n" \
"      <paths>\n" \
"        <scripts>\n" \
"          <setup_mount_dir>/etc/bivio/nrsp/scripts/scsi_mount</setup_mount_dir>\n" \
"        </scripts>\n" \
"        <mount_dir>/bivio/scsi</mount_dir>\n" \
"      </paths>\n" \
"    </scsi>\n" \
"    <paths>\n" \
"      <app_start_dir>/etc/bivio/nrsp/appstart.d</app_start_dir>\n" \
"      <conf_file>/etc/bivio/nrsp/nrsp.xml</conf_file>\n" \
"      <app_prof_dir>/etc/bivio/nrsp/profiles</app_prof_dir>\n" \
"      <binds_file>/etc/bivio/nrsp/nrsp_binds.xml</binds_file>\n" \
"      <failover_active_conf>/etc/bivio/nrsp/failover_active.xml</failover_active_conf>\n" \
"      <oem_opts_dir>/etc/bivio/nrsp/oem-opts</oem_opts_dir>\n" \
"      <redchange_log>/var/log/bivio/redchange.cf</redchange_log>\n" \
"      <scripts>\n" \
"        <redchange>/etc/bivio/nrsp/scripts/redchange</redchange>\n" \
"        <postinv>/etc/bivio/nrsp/scripts/postinv</postinv>\n" \
"        <redpeerchange>/etc/bivio/nrsp/scripts/redpeerchange</redpeerchange>\n" \
"        <postchange>/etc/bivio/nrsp/scripts/postchange</postchange>\n" \
"        <poststartup>/etc/bivio/nrsp/scripts/poststartup</poststartup>\n" \
"        <dir>/etc/bivio/nrsp/scripts</dir>\n" \
"      </scripts>\n" \
"      <conf_file_override>/etc/bivio/nrsp/nrsp_override.xml</conf_file_override>\n" \
"      <aux_npc_file>/bivio/local/AUX-NPC</aux_npc_file>\n" \
"      <init_dir>/etc/bivio/init.d</init_dir>\n" \
"      <npc_file>/bivio/local/NPC</npc_file>\n" \
"      <cpulists>/var/lib/bivio/nrsp/cpulists</cpulists>\n" \
"      <failover_boot_conf>/etc/bivio/nrsp/failover_boot.xml</failover_boot_conf>\n" \
"      <app_off_dir>/etc/bivio/nrsp/appstart.off</app_off_dir>\n" \
"      <events>/etc/bivio/nrsp/events</events>\n" \
"    </paths>\n" \
"    <bvsp>\n" \
"      <settings>\n" \
"        <db_host>local-xpc</db_host>\n" \
"        <db_user>bvsp</db_user>\n" \
"        <db_pass>bvsp</db_pass>\n" \
"        <db_name>bvspmemdb</db_name>\n" \
"      </settings>\n" \
"    </bvsp>\n" \
"  </nrsp>\n" \
"  <net>\n" \
"    <staticroutes>\n" \
"    </staticroutes>\n" \
"    <accounts>\n" \
"      <user username=\"admin\">\n" \
"          <role>admin</role>\n" \
"      </user>\n" \
"    </accounts>\n" \
"  </net>\n" \
"  <system>\n" \
"    <mccp>\n" \
"       <subnet>10.10.10.0</subnet>\n" \
"    </mccp>\n" \
"    <id>\n" \
"       <sysname>unknown</sysname>\n" \
"    </id>\n" \
"    <logs>\n" \
"      <log name=\"defaults\">\n" \
"        <dir>/var/log</dir>\n" \
"        <local>root</local>\n" \
"        <scsipart/>\n" \
"        <forceflat>no</forceflat>\n" \
"        <class>*.*</class>\n" \
"        <type>cf</type>\n" \
"        <cfsize>15000000</cfsize>\n" \
"        <cffill>1</cffill>\n" \
"      </log>\n" \
"      <log name=\"system\" syslog=\"yes\">\n" \
"        <dir>/var/log</dir>\n" \
"        <file>messages</file>\n" \
"        <local>root</local>\n" \
"        <scsipart></scsipart>\n" \
"        <forceflat>yes</forceflat>\n" \
"        <remote>192.168.120.89</remote>\n" \
"        <class>*.*</class>\n" \
"        <type>flat</type>\n" \
"        <cfsize>15000000</cfsize>\n" \
"        <cffill>1</cffill>\n" \
"      </log>\n" \
"      <log name=\"rsync\" syslog=\"no\">\n" \
"        <dir>/var/log</dir>\n" \
"        <file>rsyncd</file>\n" \
"        <forceflat>no</forceflat>\n" \
"        <local>root</local>\n" \
"        <type>cf</type>\n" \
"        <class>~^rsyncd *.*</class>\n" \
"        <cfsize>500000</cfsize>\n" \
"        <cffill>1</cffill>\n" \
"      </log>\n" \
"      <log name=\"event\" syslog=\"no\">\n" \
"        <dir>/var/log</dir>\n" \
"        <file>events</file>\n" \
"        <forceflat>no</forceflat>\n" \
"        <local>root</local>\n" \
"        <type>cf</type>\n" \
"        <class>~^nreventmgrd, \\\"NR Event\\\" *.*</class>\n" \
"        <cfsize>1000000</cfsize>\n" \
"        <cffill>1</cffill>\n" \
"      </log>\n" \
"      <log name=\"alarm\" syslog=\"no\">\n" \
"        <dir>/var/log</dir>\n" \
"        <file>alarms</file>\n" \
"        <forceflat>no</forceflat>\n" \
"        <local>root</local>\n" \
"        <type>cf</type>\n" \
"        <class>^nreventmgrd, \\\"NR Alarm\\\" *.*</class>\n" \
"        <cfsize>1000000</cfsize>\n" \
"        <cffill>1</cffill>\n" \
"      </log>\n" \
"      <log name=\"software\">\n" \
"        <dir>/var/log/bivio</dir>\n" \
"        <file>software</file>\n" \
"        <forceflat>no</forceflat>\n" \
"        <local>root</local>\n" \
"        <type>cf</type>\n" \
"      </log>\n" \
"      <log name=\"backup\">\n" \
"        <dir>/var/log/bivio</dir>\n" \
"        <file>backup</file>\n" \
"        <forceflat>no</forceflat>\n" \
"        <local>root</local>\n" \
"        <type>cf</type>\n" \
"      </log>\n" \
"      <log name=\"routing\">\n" \
"        <dir>/var/log/bivio</dir>\n" \
"        <file>routing</file>\n" \
"        <forceflat>no</forceflat>\n" \
"        <local>root</local>\n" \
"        <type>cf</type>\n" \
"      </log>\n" \
"      <log name=\"bvcig\">\n" \
"        <dir>/var/log/bivio</dir>\n" \
"        <file>bvcig</file>\n" \
"        <forceflat>no</forceflat>\n" \
"        <local>root</local>\n" \
"        <type>cf</type>\n" \
"      </log>\n" \
"    </logs>\n" \
"    <service name=\"TELNETD\">\n" \
"      <start>yes</start>\n" \
"      <interface>all</interface>\n" \
"      <server>/usr/sbin/in.telnetd</server>\n" \
"    </service>\n" \
"    <service name=\"FTPD\">\n" \
"      <start>yes</start>\n" \
"      <interface>all</interface>\n" \
"      <server>/usr/sbin/in.ftpd</server>\n" \
"    </service>\n" \
"    <service name=\"SSHD\">\n" \
"      <start>yes</start>\n" \
"    </service>\n" \
"    <service name=\"XINETD\">\n" \
"      <start>yes</start>\n" \
"    </service>\n" \
"    <service name=\"INETD\">\n" \
"      <start>no</start>\n" \
"    </service>\n" \
"    <logrotate>\n" \
"      <maxsize>2097152</maxsize>\n" \
"    </logrotate>\n" \
"  </system>\n" \
"  <oem-opt>\n" \
"    <default_mode>trans-only</default_mode>\n" \
"    <iobp>0</iobp>\n" \
"  </oem-opt>\n" \
"</bivio>\n" };

/*
 * Null compare function -- always returns FALSE so an element is always
 * inserted into a hash table (i.e. there is never a collision with an
 * existing element).
 */

static boolean nullcmp(hash_datum *d1, hash_datum *d2)
{
        return 0;
}

static void freeptr(hash_datum *d1)
{
	BCON_CONFIG *bcon_config = (BCON_CONFIG *)d1;
	free(bcon_config);
}

static void freetrptr(hash_datum *d1)
{
	TRANSACTION_TYPE *trans_type = (TRANSACTION_TYPE *)d1;
	free(trans_type->hash_key);
	free(trans_type->node_key);
	free(trans_type->remove_key);
	bcon_node_free(trans_type->data);
	free(trans_type);
}

static boolean handlecmp(hash_datum *d1, hash_datum *d2)
{
        int *hndl_1 = (int *) d1;
        BCON_CONFIG *hndl_2 = (BCON_CONFIG *) d2;

	//printf("Compare called: d1 = %d, d2 = %d\n", *hndl_1, hndl_2->id);

        return (*hndl_1 == hndl_2->handle);
}

static boolean handletrcmp(hash_datum *d1, hash_datum *d2)
{
        void *hndl_1 = (void *) d1;
        TRANSACTION_TYPE *hndl_2 = (TRANSACTION_TYPE *) d2;

	//printf("Compare called: d1 = %s, d2 = %s\n", (char *)hndl_1, (char *)hndl_2->hash_key);

        return (strcmp((char*)hndl_1, (char *)hndl_2->hash_key) ? 0 : 1);
}


/******************************************************************************
 *      bcon_cfg_write_lockfile
 ******************************************************************************
 *
 *      Create the lock file
 *
 *      Returns: 0 on success, or -1 on error
 *      ** Sets bvsp_errno flag **
 ******************************************************************************/
static int
bcon_cfg_write_lockfile(char *lockfile, int *lock_fd)
{
        char            buf[256];
        long            tm;
        int             ret;
        int             fd;

       	/* Lets create the lock file since we're saving */
       	fd = open(lockfile,   O_RDWR|O_CREAT|O_TRUNC|O_SYNC,
               	                        (S_IRUSR|S_IWUSR) |
                       	                (S_IRGRP) |
                               	        (S_IROTH));
       	if (fd < 0) {
               	return bcon_return_error(BVSP_ERR_OPEN_FAILED);
       	}
        time(&tm);
        sprintf(buf, "Config Locked\nPID %ld\nLocked At %s",
                                           (unsigned long)getpid(), ctime(&tm));
        ret = write(fd, buf, strlen(buf));
        if (ret != (int)strlen(buf)) {
                close(fd);
                unlink(lockfile);
                return bcon_return_error(BVSP_ERR_WRITE_FAILED);
        }
        *lock_fd = fd;

        return bcon_return_success();
}



/******************************************************************************
 *      bcon_cfg_remove_lockfile
 ******************************************************************************
 *
 *      Remove the lock file
 *
 *      Returns: 0 on success, or -1 on error
 *      ** Sets bvsp_errno flag **
 ******************************************************************************/
static int
bcon_cfg_remove_lockfile(char *lockfile, int *lock_fd)
{                                          
        if (*lock_fd < 0) {
                return bcon_return_error(BVSP_ERR_NOT_FOUND);
        }       
                
        close(*lock_fd);
        unlink(lockfile);
        *lock_fd = -1;

        return bcon_return_success();
}



/******************************************************************************
 *      bcon_cfg_check_lockfile
 ******************************************************************************
 *
 *      Check the lock file
 *
 *      Returns: 0 if not locked, 1 if locked, -1 on error
 *      ** Sets bvsp_errno flag **
 ******************************************************************************/
static int
bcon_cfg_check_lockfile(int block, char *lockfile)
{
        struct dirent   *dent;
        u_long          pid_no;
        FILE            *f1;
        char            str[256];
        char            pid_dir[1024];
        char            sock_file[1024];
        char            link_file[1024];
        char            *tmp_fn1;
        char            *tmp_fn2;
        DIR             *dir;
        int             lock_timeout = (block) ? LOCK_TIMEOUT_SECONDS : 1;
        int             ret;

        while(lock_timeout > 0) {
                f1 = fopen(lockfile, "rt");
                if (!f1) {
                        return bcon_return_success();
                }

                if (!fgets(str, 256, f1)) {    /* Desc - skip */
                        /* ignore failure */
                }
                if (!fgets(str, 256, f1)) {    /* PID ID */
                        /* ignore failure */
                }
                fclose(f1);

                pid_no = atol(&str[4]);
                ret = kill(pid_no, 0);
                if (ret < 0 && errno == ESRCH) {
                        goto stale;
                }
                sprintf(pid_dir, "/proc/%ld/fd", pid_no);
                dir = opendir(pid_dir);
                if (!dir) {
                        /*
                         * We can't check pid, maybe no proc?
                         * just assume it's locked
                         */
                        goto locked;
                }

                tmp_fn1 = strdup(basename(lockfile));
                while((dent = readdir(dir))) {
                        if (!isdigit(*dent->d_name)) {
                                continue;
                        }

                        sprintf(sock_file, "%s/%d",
                                                   pid_dir, atoi(dent->d_name));
                        ret = readlink(sock_file, link_file, sizeof(link_file));
                        if (ret < 0) {
                                continue;
                        }
                        else if (ret == sizeof(link_file)) {
                                link_file[sizeof(link_file)-1] = '\0';
                        }

                        /*
                         * Since we don't get the *real* path to a file through
                         * a symbolic link, we have to check base names.  This
                         * can cause certain scenarios to fail, but the best
                         * I can come up with for now.
                         */
                        tmp_fn2 = strdup(basename(link_file));
                        if (!strcmp(tmp_fn1, tmp_fn2)) {
                                closedir(dir);
                                free(tmp_fn1);
                                free(tmp_fn2);
                                goto locked;
                        }
                        free(tmp_fn2);
                }
                closedir(dir);
                free(tmp_fn1);
                /*      
                 * If we got here, there were no entries/matches, so the
                 * lockfile must be stale
                 */     
                goto stale;     
                        
locked:
                if (block) {
                       	sleep(1);
                }        
                lock_timeout--;
        }                
                         
        /* If we made it here, we timed out -- return that it is locked */
        bcon_set_success();
        return(1); /* locked */
                                
                                
stale:                          
        /* Stale lock -- force unlock it */
        unlink(lockfile);
        bcon_set_success();
        return(0); /* not locked */
}               

const char *bcon_get_last_output_buffer( void ) {
	if(default_bcon_handle != 0) {
		BCON_CONFIG *bcon_config = bcon_get_config(default_bcon_handle);
		if(bcon_config != NULL)
			return (bcon_config->last_output_buffer ? bcon_config->last_output_buffer : "");
	}

	return "";
}

void bcon_set_last_output_buffer( const char *error_buffer ) {
	if(default_bcon_handle != 0) {
		BCON_CONFIG *bcon_config = bcon_get_config(default_bcon_handle);
		if(bcon_config != NULL) {
			if(bcon_config->last_output_buffer != NULL)
				free(bcon_config->last_output_buffer);

			bcon_config->last_output_buffer = strdup(error_buffer);
		}
	}
}

const char *bcon_get_last_output_buffer_error( void ) {
	if(default_bcon_handle != 0) {
		BCON_CONFIG *bcon_config = bcon_get_config(default_bcon_handle);
		if(bcon_config != NULL)
			return bcon_config->last_output_buffer_error;
	}

	return "";
}

int bcon_get_last_output_error( void ) {
	if(default_bcon_handle != 0) {
		BCON_CONFIG *bcon_config = bcon_get_config(default_bcon_handle);
		if(bcon_config != NULL)
			return bcon_config->last_output_error_code;
	}

	return 0;
}

int bcon_get_last_error( void )
{
	return bcon_get_last_error_value();
}

int bcon_clear_last_error( void )
{
	return bcon_clear_last_error_value();
}

const char * bcon_get_error_str( int lastErrno )
{
	const char *ErrorStr = "UNKNOWN";
        const ErrorConstant *next;

        for ( next = bcon_errors; NULL != next->description; ++next )
        {
                if ( next->value == lastErrno ) {
			ErrorStr = next->description;

			break;	
		}
        }

        return ErrorStr;
}

const char * bcon_get_last_error_str( void )
{
	return bcon_get_error_str( bcon_get_last_error() );
}

void bcon_free_xml_config( BConCfgPtr cfg )
{
	if(cfg != NULL) {
		xmlFreeDoc(cfg);

		xmlCleanupParser();
	}
}

void bcon_save_config(BCON_CONFIG *bcon_config)
{
	if(bcon_config != NULL && bcon_config->db_cfg != NULL) {
		//xmlSaveFile(bcon_config->db_path, bcon_config->db_cfg);
		xmlKeepBlanksDefault(0);
		xmlIndentTreeOutput = 1;
#ifdef SAFE_SAVE	/* fix a bug */
	{
		/* BUG: we should do this safely and reject the
		 * original request if there's a problem indicated by
		 * xmlSaveFormatFile returning -1. instead, we ignore
		 * any problems, so if the disk is full, for example,
		 * we just leave a damaged file because it gets half
		 * written.
		 *
		 * I'd just roll code to fix this by saving a tmp file
		 * first and overwriting the original only if the save
		 * works, but notice that there's no error return
		 * here. I'd have to rework all the places this is
		 * called to handle a failure indication from this
		 * source. That doesn't look too bad, although it
		 * raises an annoying question: this routine is often
		 * called even though the action *already failed*.
		 * Look at the code yourself if you don't believe me.
		 * What's that about?
		 * -dprovan 11/1/12.
		 */
		char	tmpname[maximum file name];

		/* I hope it fits */
		snprintf(tmpname, sizeof tmpname, "%s.tmp",
			 bcon_config->db_path);

		/* new version is OK. don't worry about avoiding the
		 * tmp/rename song a dance when there's no original
		 * file to preserve, since we're rarely creating a
		 * new file here.
		 */
		if (xmlSaveFormatFile(tmpname, bcon_config->db_cfg, 1) <= 0) {
			/* output failed including zero bytes written */
			unlink(tmpname);
			return error;
		}

		if (rename(tmpname, bcon_config->db_path) < 1) {
			/* drat. sooo close */
			unlink(tmpname);
			return error.
		}
	}
#else	/* go ahead and be that way: save file in a way that can fail
	 * catastrophically.
	 */
		xmlSaveFormatFile(bcon_config->db_path, bcon_config->db_cfg, 1);
#endif

        	struct stat cfg_stat;
        	if(stat(bcon_config->db_path, &cfg_stat) >= 0) {
			bcon_config->db_mtime = cfg_stat.st_mtime;
        	}
	}
}


BCON_CONFIG *get_handle(int db_handle)
{
        unsigned hashcode = hash_HashFunction((const u_char *) &db_handle, sizeof(int));
        BCON_CONFIG *bcon_config = (BCON_CONFIG *)hash_Lookup(db_hash_table, hashcode, handlecmp, &db_handle);
	if(bcon_config != NULL) {
		struct stat cfg_stat;
		if(bcon_config->db_path != NULL &&
		   stat(bcon_config->db_path, &cfg_stat) >= 0) {
			if(bcon_config->db_cfg == NULL || bcon_config->db_mtime != cfg_stat.st_mtime) {
				int got_lock = 1;
				if(bcon_config->lock != NULL && bcon_config->lock_fd < 0 && bcon_config->db_cfg != NULL) {
        				int ret = bcon_cfg_check_lockfile(1, bcon_config->lock);
	        			if ((ret != 0) ||
					    (bcon_cfg_write_lockfile(bcon_config->lock, &bcon_config->lock_fd) != 0)) {
                				got_lock = 0;
					}
				}
				if(got_lock) {
					bcon_free_xml_config(bcon_config->db_cfg);
					xmlKeepBlanksDefault(0);
					bcon_config->db_cfg = xmlParseFile(bcon_config->db_path);
					bcon_config->db_mtime = cfg_stat.st_mtime;
					if(bcon_config->lock != NULL)
						bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);
				}
			}
		} else if(bcon_config->db_cfg != NULL) {
			bcon_free_xml_config(bcon_config->db_cfg);
			bcon_config->db_cfg = NULL;
			bcon_config->db_mtime = (time_t)0;
		}

		if(bcon_config->trigger_db_path != NULL &&
		   stat(bcon_config->trigger_db_path, &cfg_stat) >= 0) {
			if(bcon_config->trigger_db_cfg == NULL || bcon_config->trigger_db_mtime != cfg_stat.st_mtime) {
				bcon_free_xml_config(bcon_config->trigger_db_cfg);
				xmlKeepBlanksDefault(0);
				bcon_config->trigger_db_cfg = xmlParseFile(bcon_config->trigger_db_path);
				bcon_config->trigger_db_mtime = cfg_stat.st_mtime;
			}
		} else if(bcon_config->trigger_db_cfg != NULL) {
			bcon_free_xml_config(bcon_config->trigger_db_cfg);
			bcon_config->trigger_db_cfg = NULL;
			bcon_config->trigger_db_mtime = (time_t)0;
		}
	}

	return bcon_config;
}

/* bcon_get_config

   get the bcon configuration
*/
BCON_CONFIG *bcon_get_config(int bcon_handle)
{
	return get_handle(bcon_handle);
}


int free_handle(int db_handle)
{
	int ret = BVSP_ERR_NOT_FOUND;

        unsigned hashcode = hash_HashFunction((const u_char *) &db_handle, sizeof(int));
        BCON_CONFIG *bcon_config = (BCON_CONFIG *)hash_Lookup(db_hash_table, hashcode, handlecmp, &db_handle);
	if(bcon_config != NULL) {
		free(bcon_config->db_path);
		free(bcon_config->trigger_db_path);
		bcon_free_xml_config(bcon_config->db_cfg);
		bcon_free_xml_config(bcon_config->trigger_db_cfg);
		bcon_config->db_cfg = NULL;
		bcon_config->trigger_db_cfg = NULL;

		free(bcon_config->last_error);
		free(bcon_config->last_output_buffer);
		free(bcon_config->last_output_buffer_error);
		free(bcon_config->parent_key);
		bcon_xml_free_copy(bcon_config->child_nodes);

		bcon_config->parent_key = NULL;
		bcon_config->child_nodes = NULL;

		free(bcon_config->lock);
		bcon_config->lock = NULL;

		if(bcon_config->tr_hash_table != NULL) {
			hash_Reset(bcon_config->tr_hash_table, freetrptr);

			free(bcon_config->tr_hash_table);
			bcon_config->tr_hash_table = NULL;
		}

		if(bcon_config->trans_data != NULL) {
			int idx;
			for(idx=0; idx<bcon_config->trans_count; ++idx) {
				free(bcon_config->trans_data[idx]);
			}
			free(bcon_config->trans_data);
			bcon_config->trans_data = NULL;
			bcon_config->trans_count = -1;
		}

		hash_Delete(db_hash_table, hashcode, handlecmp, &db_handle, freeptr);

		//free(bcon_config);

		ret = BVSP_ERR_SUCCESS;
	}

	bcon_set_error(ret);

	return ret;
}

/* bcon_free_config

   free the bcon configuration
*/
int bcon_free_config(int bcon_handle)
{
	return free_handle(bcon_handle);
}


BCON_CONFIG *init_handle(BCON_CONFIG *bcon_config)
{
	if(db_hash_table == NULL) {
		db_hash_table = hash_Init(INIT_HASH_TABLE_SIZE);
	}

	if(bcon_config->tr_hash_table == NULL) {
		bcon_config->tr_hash_table = hash_Init(INIT_HASH_TABLE_SIZE);
	}

	unsigned hashcode = hash_HashFunction((const u_char *)&(bcon_config->handle), sizeof(int));
	boolean not_inserted = hash_Insert(db_hash_table, hashcode, nullcmp, &(bcon_config->handle), bcon_config);
	if(not_inserted) {
		free_handle(bcon_config->handle);

		bcon_config = NULL;
	}

	return bcon_config;
}


/* bcon_init_config

   init the bcon configuration
*/
int bcon_init_config(const char *configfile, const char *triggerfile)
{
	BCON_CONFIG *bcon_config = NULL;
                
	if(((configfile != NULL) && (strlen(configfile) > 0)) ||
	   ((triggerfile != NULL) && (strlen(triggerfile) > 0))) {
		time_t time_one = (time_t)0;
		time_t time_two = (time_t)0;
		struct stat cfg_stat;
		if((configfile != NULL) && (strlen(configfile) > 0)) {
			if(stat((char *)configfile, &cfg_stat) < 0) {
				int fd;
				int ret = -1;

				if(!strcmp(configfile, BCON_CONFIG_FILE)) {
					fd = open(BCON_CONFIG_FILE, O_RDONLY);
					if (fd < 0) {
						fd = open(BCON_CONFIG_FILE, O_WRONLY | O_CREAT, 0644);
						if(fd >= 0) {
							ret = write(fd, Default_Xml_Config, strlen(Default_Xml_Config));
							close(fd);
						}
					} else {
						close(fd);
					}
				}

				if(ret < 0) {
					bcon_set_error(BVSP_ERR_OPEN_FAILED);

					return 0;
				} else {
					if(stat((char *)configfile, &cfg_stat) < 0)
						time_one = 0;
					else
						time_one = cfg_stat.st_mtime;
				}
			} else {
				time_one = cfg_stat.st_mtime;
			}
		}
		if((triggerfile != NULL) && (strlen(triggerfile) > 0)) {
			if(stat((char *)triggerfile, &cfg_stat) < 0) {
				/* for now ignore error */
				//bcon_set_error(BVSP_ERR_OPEN_FAILED);

				//return 0;

				triggerfile = NULL;
			} else {
				time_two = cfg_stat.st_mtime;
			}
		}

		next_handle += 1;
		bcon_config = malloc(sizeof(BCON_CONFIG)); 
		bcon_config->handle = next_handle;
		bcon_config->db_mtime = (time_t)0;
		bcon_config->trigger_db_mtime = (time_t)0;
		bcon_config->lock_fd = -1;
		if(((configfile != NULL) && (strlen(configfile) > 0)) && 
           	   ((triggerfile != NULL) && (strlen(triggerfile) > 0))) {
			bcon_config->type = BCON_BOTH_DB;
			bcon_config->db_path = strdup(configfile);
			bcon_config->trigger_db_path = strdup(triggerfile);
			bcon_config->db_mtime = time_one;
			bcon_config->trigger_db_mtime = time_two;

			bcon_config->lock = malloc(strlen(configfile) + strlen(lock_prefix) + 1);
			strcpy(bcon_config->lock, configfile);
			strcat(bcon_config->lock, lock_prefix);
		} else if((configfile != NULL) && (strlen(configfile) > 0)) {
			bcon_config->type = BCON_CONFIG_DB;
			bcon_config->db_path = strdup(configfile);
			bcon_config->trigger_db_path = NULL;
			bcon_config->db_mtime = time_one;

			bcon_config->lock = malloc(strlen(configfile) + strlen(lock_prefix) + 1);
			strcpy(bcon_config->lock, configfile);
			strcat(bcon_config->lock, lock_prefix);
		} else {
			bcon_config->type = BCON_TRIGGER_CONFIG_DB;
			bcon_config->db_path = NULL;
			bcon_config->trigger_db_path = strdup(triggerfile);
			bcon_config->trigger_db_mtime = time_two;
		}
		bcon_config->db_mtime = (time_t)0;
		bcon_config->trigger_db_mtime = (time_t)0;
		bcon_config->db_cfg = NULL;
		bcon_config->trigger_db_cfg = NULL;
		bcon_config->last_error_code = 0;
		bcon_config->last_error = NULL;
		bcon_config->last_output_buffer = NULL;
		bcon_config->last_output_buffer_error = NULL;
		bcon_config->last_output_error_code = 0;
		bcon_config->parent_key = NULL;
		bcon_config->child_nodes = NULL;
		bcon_config->tr_hash_table = NULL;
		bcon_config->trans_data = NULL;
		bcon_config->trans_count = -1;

		bcon_config = init_handle(bcon_config);
	}

	if(bcon_config != NULL)
		bcon_set_error(BVSP_ERR_SUCCESS);
	else
		bcon_set_error(BVSP_ERR_FAILED);

        return (bcon_config != NULL ? bcon_config->handle : 0);
}

/* bcon_init

   initialize the configuration library
*/
int bcon_init(void)
{
	if(default_bcon_handle != 0) {
		bcon_free_config(default_bcon_handle);

		default_bcon_handle = 0;
	}

	default_bcon_handle = bcon_init_config(BCON_CONFIG_FILE, BCON_TRIGGER_FILE);
	if(default_bcon_handle != 0) {
		default_bcon_config = bcon_get_config(default_bcon_handle);
	}

	if(default_bcon_handle != 0)
		bcon_initialized = BVSP_ERR_SUCCESS;
	else
		bcon_initialized = BCON_INITIALIZATION_FAILED;

	bcon_set_error(bcon_initialized);

	return bcon_initialized;
}

/* bcon_init_db

   initialize the db configuration library
*/
int bcon_init_db(const char *configfile, const char *triggerfile)
{
	int bcon_db_initialized;

	int bcon_db_handle = bcon_init_config(configfile, triggerfile);
	if(bcon_db_handle != 0)
		bcon_db_initialized = BVSP_ERR_SUCCESS;
	else
		bcon_db_initialized = BCON_INITIALIZATION_FAILED;

	bcon_set_error(bcon_db_initialized);

	return bcon_db_handle;
}

/* bcon_is_init

   check initialize of the configuration library
*/
int bcon_is_init(void)
{
	return bcon_initialized;
}

/* bcon_is_init_db

   check initialize of the db configuration library
*/
int bcon_is_init_db(int bcon_handle)
{
	BCON_CONFIG *got_config = bcon_get_config(bcon_handle);

	return (got_config ? BVSP_ERR_SUCCESS : BVSP_ERR_NOT_INITIALIZED);
}


/* bcon_cleanup

   cleanup
*/
int bcon_cleanup(void)
{
	if(default_bcon_handle != 0) {
		bcon_free_config(default_bcon_handle);

		default_bcon_handle = 0;

		default_bcon_config = NULL;
	}

	bcon_initialized = BVSP_ERR_NOT_INITIALIZED;

	return BVSP_RETURN_SUCCESS();
}


/* bcon_cleanup_db

   cleanup db
*/
int bcon_cleanup_db(int bcon_handle)
{
	if(bcon_handle != 0) {
		bcon_free_config(bcon_handle);
	}

	return BVSP_RETURN_SUCCESS();
}


/* allocates memory for children. it's caller's repsonsibility to free */
char* (*bcon_get_children(const char *key, int *pcount))[]
{
	int free_default_bcon_handle = 0;

	bcon_init_success();

	char*	(*p_children)[];

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	p_children = bcon_get_children_db(default_bcon_handle, key, pcount);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return p_children;
}


/* allocates memory for children. it's caller's repsonsibility to free */
char* (*bcon_get_children_db_type(int bcon_handle, int db_type, const char *key, int *pcount))[]
{
	bcon_init_success();

	char*	(*p_children)[];
	char    xpath[BCON_MAX_KEY_SIZE];
	//int	ret;

	if (!pcount) {
		bcon_set_error(BVSP_ERR_INVALID_ARG);
		return NULL;
	}

	BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
	if (!bcon_config ||
	    ((db_type == DB_STORE_TYPE && bcon_config->db_cfg == NULL) || 
	     (db_type == DB_TRIGGER_TYPE && bcon_config->trigger_db_cfg == NULL))) {
		bcon_set_error(BVSP_ERR_DECODE_FAILED);
		return NULL;
	}

	xstrncpy(xpath, key, sizeof (xpath));
	strcat(xpath, "*");
	p_children = bcon_xml_xpath_nodes((xmlDocPtr)(db_type == DB_STORE_TYPE ? 
					   bcon_config->db_cfg : bcon_config->trigger_db_cfg), 
					   (const xmlChar *)xpath, pcount);

	return p_children;
}


/* allocates memory for children. it's caller's repsonsibility to free */
char* (*bcon_get_children_db(int bcon_handle, const char *key, int *pcount))[]
{
	return bcon_get_children_db_type(bcon_handle, DB_STORE_TYPE, key, pcount);
}


/* allocates memory for children. it's caller's repsonsibility to free */
char* (*bcon_get_children_attr(const char *key, int *pcount))[]
{
	int free_default_bcon_handle = 0;

	bcon_init_success();

	char*	(*p_children)[];

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	p_children = bcon_get_children_attr_db(default_bcon_handle, key, pcount);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return p_children;
}


/* allocates memory for children. it's caller's repsonsibility to free */
char* (*bcon_get_children_attr_db_by_type(int bcon_handle, int db_type, const char *key, int *pcount))[]
{
	bcon_init_success();

	char*	(*p_children)[];
	char    xpath[BCON_MAX_KEY_SIZE];
	//int	ret;

	if (!pcount) {
		bcon_set_error(BVSP_ERR_INVALID_ARG);
		return NULL;
	}

	BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
	if (!bcon_config ||
	    ((db_type == DB_STORE_TYPE && bcon_config->db_cfg == NULL) || 
	     (db_type == DB_TRIGGER_TYPE && bcon_config->trigger_db_cfg == NULL))) {
		bcon_set_error(BVSP_ERR_DECODE_FAILED);
		return NULL;
	}

	xstrncpy(xpath, key, sizeof (xpath));
	p_children = bcon_xml_xpath_nodes((xmlDocPtr)(db_type == DB_STORE_TYPE ?
                                           bcon_config->db_cfg : bcon_config->trigger_db_cfg),
					   (const xmlChar *)xpath, pcount);

	return p_children;
}


/* allocates memory for children. it's caller's repsonsibility to free */
char* (*bcon_get_children_attr_db(int bcon_handle, const char *key, int *pcount))[]
{
	return bcon_get_children_attr_db_by_type(bcon_handle, DB_STORE_TYPE, key, pcount);
}


void bcon_free_children(char* (*p)[], int count)
{
	bcon_init_success();

	while (count-- > 0) {
		if ((*p)[count])
			free((*p)[count]);
	}
	free(p);
}



/* bcon_get_count

   get the count of a particular key
*/
int bcon_get_count(const char *key)
{
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	int ret = bcon_get_count_db(default_bcon_handle, key);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}


/* bcon_get_count_db

   get the count of a particular key
*/
int bcon_get_count_db_by_type(int bcon_handle, int db_type, const char *key)
{
	bcon_init_success();

	int ret;
	BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
	if (!bcon_config ||
	    ((db_type == DB_STORE_TYPE && bcon_config->db_cfg == NULL) || 
	     (db_type == DB_TRIGGER_TYPE && bcon_config->trigger_db_cfg == NULL))) {
		bcon_set_error(BVSP_ERR_DECODE_FAILED);
		return -1;
	}

	ret = bcon_xml_get_count((xmlDocPtr)(db_type == DB_STORE_TYPE ?
                                           bcon_config->db_cfg : bcon_config->trigger_db_cfg),
					   (const xmlChar *)key);

	return ret;
}


int bcon_get_count_db(int bcon_handle, const char *key)
{
	return bcon_get_count_db_by_type(bcon_handle, DB_STORE_TYPE, key);
}


/* bcon_get

   get the value of a particular key
*/
int bcon_get(const char *key, void *val, int len)
{
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	int ret = bcon_get_db(default_bcon_handle, key, val, len);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}


/* bcon_get_db

   get the value of a particular key
*/
int bcon_get_db_by_type(int bcon_handle, int db_type, const char *key, void *val, int len)
{
	bcon_init_success();

	int ret;
	BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
	if (!bcon_config ||
	    ((db_type == DB_STORE_TYPE && bcon_config->db_cfg == NULL) || 
	     (db_type == DB_TRIGGER_TYPE && bcon_config->trigger_db_cfg == NULL))) {
		bcon_set_error(BVSP_ERR_DECODE_FAILED);
		return -1;
	}

	ret = bcon_xml_get((xmlDocPtr)(db_type == DB_STORE_TYPE ?
                                           bcon_config->db_cfg : bcon_config->trigger_db_cfg),
					   (const xmlChar *)key, val, len);

	return ret;
}


int bcon_get_db(int bcon_handle, const char *key, void *val, int len)
{
	return bcon_get_db_by_type(bcon_handle, DB_STORE_TYPE, key, val, len);
}


int bcon_get_ptr(const char *key, void **pval)
{
	bcon_init_success();

	int len;
	if (!pval) {
		bcon_set_error(BVSP_ERR_INVALID_ARG);
		return -1;
	}

	*pval = malloc(len = BCON_MAX_CONFIG_LINE_SIZE);
	if (!*pval) {
		bcon_set_error(BVSP_ERR_NO_MEMORY);
		return -1;
	}

	return bcon_get(key, *pval, len);
}


int bcon_get_ptr_db(int bcon_handle, const char *key, void **pval)
{
	bcon_init_success();

	int len;
	if (!pval) {
		bcon_set_error(BVSP_ERR_INVALID_ARG);
		return -1;
	}

	*pval = malloc(len = BCON_MAX_CONFIG_LINE_SIZE);
	if (!*pval) {
		bcon_set_error(BVSP_ERR_NO_MEMORY);
		return -1;
	}

	return bcon_get_db(bcon_handle, key, *pval, len);
}


void bcon_free_ptr(void *p)
{
	bcon_init_success();

	if (!p) {
		bcon_set_error(BVSP_ERR_INVALID_ARG);
		return;
	}

	free(p);
	bcon_set_success();
}


int bcon_get_int(const char *key, int *i)
{
	bcon_init_success();

	char val[32];

	if (!i) {
		return bcon_return_error(BVSP_ERR_INVALID_ARG);
	}

	if (bcon_get(key, (void *)val, sizeof (val)) < 0) {
		return -1;
	}
	sscanf(val, "%d", i);
	return bcon_return_success();
}


int bcon_get_int_db(int bcon_handle, const char *key, int *i)
{
	bcon_init_success();

	char val[32];

	if (!i) {
		return bcon_return_error(BVSP_ERR_INVALID_ARG);
	}

	if (bcon_get_db(bcon_handle, key, (void *)val, sizeof (val)) < 0) {
		return -1;
	}
	sscanf(val, "%d", i);
	return bcon_return_success();
}


int bcon_get_double(const char *key, double *d)
{
	char val[32];

	if (!d) {
		return bcon_return_error(BVSP_ERR_INVALID_ARG);
	}

	if (bcon_get(key, (void *)val, sizeof (val)) < 0) {
		return -1;
	}

	sscanf(val, "%lf", d);
	return bcon_return_success();
}


int bcon_get_double_db(int bcon_handle, const char *key, double *d)
{
	char val[32];

	if (!d) {
		return bcon_return_error(BVSP_ERR_INVALID_ARG);
	}

	if (bcon_get_db(bcon_handle, key, (void *)val, sizeof (val)) < 0) {
		return -1;
	}

	sscanf(val, "%lf", d);
	return bcon_return_success();
}


int bcon_get_str(const char *key, char *s, int len)
{
	bcon_init_success();

	return bcon_get(key, (void *)s, len);
}


int bcon_get_str_db(int bcon_handle, const char *key, char *s, int len)
{
	bcon_init_success();

	return bcon_get_db(bcon_handle, key, (void *)s, len);
}


char *bcon_get_memstr(const char *key)
{
	bcon_init_success();

	void *s = NULL;
	if (bcon_get_ptr(key, &s) < 0) {
		return NULL;
	}
	return (char *)s;
}


char *bcon_get_memstr_db(int bcon_handle, const char *key)
{
	bcon_init_success();

	void *s = NULL;
	if (bcon_get_ptr_db(bcon_handle, key, &s) < 0) {
		return NULL;
	}
	return (char *)s;
}


char *bcon_get_memstr_static(const char *key)
{
	bcon_init_success();

	static char *s = NULL;

	if (s) {
		bcon_free_ptr(s);
		s = NULL;
	}

	return s = bcon_get_memstr(key);
}


char *bcon_get_memstr_static_db(int bcon_handle, const char *key)
{
	bcon_init_success();

	static char *s = NULL;

	if (s) {
		bcon_free_ptr(s);
		s = NULL;
	}

	return s = bcon_get_memstr_db(bcon_handle, key);
}
				

/* bcon_set

   set a key to a value in the currently selected config
*/
int bcon_set(const char *key, const void *val, int len)
{
	return bcon_set_by_action(SAVE_WITH_TRIGGER, key, val, len);
}


/* bcon_set_no_trigger

   set a key to a value in the currently selected config
*/
int bcon_set_no_trigger(const char *key, const void *val, int len)
{
	return bcon_set_by_action(SAVE_WITHOUT_TRIGGER, key, val, len);
}


/* bcon_set_no_save

   set a key to a value in the currently selected config
*/
int bcon_set_no_save(const char *key, const void *val, int len)
{
	return bcon_set_by_action(TRIGGER_WITHOUT_SAVE, key, val, len);
}


/* bcon_set_by_action

   set a key to a value in the currently selected config
*/
int bcon_set_by_action(int action, const char *key, const void *val, int len)
{
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	int ret = bcon_set_db_by_action(default_bcon_handle, action, key, val, len);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}


/* bcon_set_db

   set a key to a value in the currently selected config
*/
int bcon_set_db(int bcon_handle, const char *key, const void *val, int len)
{
	return bcon_set_db_by_action(bcon_handle, SAVE_WITH_TRIGGER, key, val, len);
}


/* bcon_set_db_no_trigger

   set a key to a value in the currently selected config
*/
int bcon_set_db_no_trigger(int bcon_handle, const char *key, const void *val, int len)
{
	return bcon_set_db_by_action(bcon_handle, SAVE_WITHOUT_TRIGGER, key, val, len);
}


/* bcon_set_db_no_save

   set a key to a value in the currently selected config
*/
int bcon_set_db_no_save(int bcon_handle, const char *key, const void *val, int len)
{
	return bcon_set_db_by_action(bcon_handle, TRIGGER_WITHOUT_SAVE, key, val, len);
}
				

/* bcon_set_db_by_action

   set a key to a value in the currently selected config
*/
int bcon_set_db_by_action(int bcon_handle, int action, const char *key, const void *val, int len)
{
	bcon_init_success();

	int ret = -1;
	FILE *bvsp_cfg_fp;
	struct  stat cfgstat;

	if (!(key && val) || !strlen(key)) {
		return bcon_return_error(BVSP_ERR_INVALID_ARG);
	}

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);

                return -1;
        }

	if(bcon_config->lock != NULL) {
        	ret = bcon_cfg_check_lockfile(1, bcon_config->lock);
	        if (ret < 0) {
        	        return bcon_return_error(BVSP_ERR_TIMEOUT);
        	}
	        else if (ret) {
                	return bcon_return_error(BVSP_ERR_TIMEOUT);
        	}

        	if (bcon_cfg_write_lockfile(bcon_config->lock, &bcon_config->lock_fd) != 0) {
                	return -1;      /* It sets bvsp_errno */
        	}
	}

	if (stat(bcon_config->db_path, &cfgstat)) {
		if (errno == ENOENT) {
			if (!(bvsp_cfg_fp = fopen(bcon_config->db_path, "w"))) {
				if(bcon_config->lock != NULL)
					bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

				return bcon_return_syserror(BVSP_ERR_OPEN_FAILED);
			}
			fclose(bvsp_cfg_fp);
			bvsp_cfg_fp = NULL; 
		} else {
			if(bcon_config->lock != NULL)
				bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

			return bcon_return_syserror(BVSP_ERR_FAILED);
		}
	}

	if(action != SAVE_WITHOUT_TRIGGER) {
		ret = bcon_do_trigger(TRIGGER_WITHOUT_SAVE, TRIGGER_PRE, key, val, NULL);
	}

	char * (*oldvalues)[] = NULL;
	if(action != TRIGGER_WITHOUT_SAVE) {
		if(bcon_config->trans_count != -1) {
			char hash_key[2048];
			sprintf(hash_key, "tran(%d):", bcon_config->trans_count);
			strcat(hash_key, key);
			bcon_trans_backup_nodes_db(bcon_handle, hash_key, key, TRANS_TYPE_SET);
		}
		ret = bcon_xml_set((xmlDocPtr)bcon_config->db_cfg, (const xmlChar *)key, val, len, &oldvalues);
		bcon_save_config(bcon_config);
	}

	if(bcon_config->lock != NULL)
		bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

	char *onevalue = (oldvalues == NULL ? NULL : (*oldvalues)[0]);
	if(action != SAVE_WITHOUT_TRIGGER) {
		ret = bcon_do_trigger(((onevalue != NULL || action == TRIGGER_WITHOUT_SAVE) ? TRIGGER_TYPE_SET : TRIGGER_TYPE_CREATE), TRIGGER_POST, key, val, onevalue);
	}

	bcon_free_oldvalues(oldvalues, 1);

	return ret;
}


/* bcon_set_multiple

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple(const char **key, const char **val, int *len, int num)
{
	return bcon_set_multiple_by_action(SAVE_WITH_TRIGGER, key, val, len, num);
}


/* bcon_set_multiple_no_trigger

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_no_trigger(const char **key, const char **val, int *len, int num)
{
	return bcon_set_multiple_by_action(SAVE_WITHOUT_TRIGGER, key, val, len, num);
}


/* bcon_set_multiple_no_save

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_no_save(const char **key, const char **val, int *len, int num)
{
	return bcon_set_multiple_by_action(TRIGGER_WITHOUT_SAVE, key, val, len, num);
}
				

/* bcon_set_multiple_by_action

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_by_action(int action, const char **key, const char **val, int *len, int num)
{
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	int *ret = bcon_set_multiple_db_by_action(default_bcon_handle, action, key, val, len, num);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}
				

/* bcon_set_multiple_va_list

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_va_list(int num, va_list ap)
{
	return bcon_set_multiple_va_list_by_action(SAVE_WITH_TRIGGER, num, ap);
}
				

/* bcon_set_multiple_va_list_no_trigger

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_va_list_no_trigger(int num, va_list ap)
{
	return bcon_set_multiple_va_list_by_action(SAVE_WITHOUT_TRIGGER, num, ap);
}
				

/* bcon_set_multiple_va_list_no_save

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_va_list_no_save(int num, va_list ap)
{
	return bcon_set_multiple_va_list_by_action(TRIGGER_WITHOUT_SAVE, num, ap);
}
				

/* bcon_set_multiple_va_list_by_action

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_va_list_by_action(int action, int num, va_list ap)
{
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	int *ret = bcon_set_multiple_db_va_list_by_action(default_bcon_handle, action, num, ap);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}
				

/* bcon_set_multiple_va_args

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_va_args(int num, ...)
{
	bcon_init_success();

	va_list ap;
	va_start(ap, num);
	int *ret = bcon_set_multiple_va_list_by_action(SAVE_WITH_TRIGGER, num, ap);
	va_end(ap);

	return ret;
}
				

/* bcon_set_multiple_va_args

   set a key to a value in the currently selected config
*/
int *bcon_mset(int num, ...)
{
	bcon_init_success();

	va_list ap;
	va_start(ap, num);
	int *ret = bcon_set_multiple_va_list_by_action(SAVE_WITH_TRIGGER, num, ap);
	va_end(ap);

	return ret;
}
				

/* bcon_set_multiple_va_args_no_trigger

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_va_args_no_trigger(int num, ...)
{
	bcon_init_success();

	va_list ap;
	va_start(ap, num);
	int *ret = bcon_set_multiple_va_list_by_action(SAVE_WITHOUT_TRIGGER, num, ap);
	va_end(ap);

	return ret;
}
				

/* bcon_set_multiple_va_args_no_save

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_va_args_no_save(int num, ...)
{
	bcon_init_success();

	va_list ap;
	va_start(ap, num);
	int *ret = bcon_set_multiple_va_list_by_action(TRIGGER_WITHOUT_SAVE, num, ap);
	va_end(ap);

	return ret;
}


/* bcon_set_multiple_db

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db(int bcon_handle, const char **key, const char **val, int *len, int num)
{
	return bcon_set_multiple_db_by_action(bcon_handle, SAVE_WITH_TRIGGER, key, val, len, num);
}


/* bcon_set_multiple_db_no_trigger

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_no_trigger(int bcon_handle, const char **key, const char **val, int *len, int num)
{
	return bcon_set_multiple_db_by_action(bcon_handle, SAVE_WITHOUT_TRIGGER, key, val, len, num);
}


/* bcon_set_multiple_db_no_save

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_no_save(int bcon_handle, const char **key, const char **val, int *len, int num)
{
	return bcon_set_multiple_db_by_action(bcon_handle, TRIGGER_WITHOUT_SAVE, key, val, len, num);
}


/* bcon_set_multiple_db_by_action

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_by_action(int bcon_handle, int action, const char **key, const char **val, int *len, int num)
{
	bcon_init_success();

	int *ret = NULL;
	FILE *bvsp_cfg_fp;
	struct  stat cfgstat;
	int *my_ret = malloc(2*sizeof(int));
	*my_ret = BVSP_ERR_SUCCESS;
	*(my_ret+1) = 0;

	if (!(key && val)) {
		*my_ret = bcon_return_error(BVSP_ERR_INVALID_ARG);
		return my_ret;
	}

	int key_index=0;
	for(key_index=0; key_index<num; ++key_index) {
		if(!strlen((char *)*(key+key_index))) {
			*my_ret = bcon_return_error(BVSP_ERR_INVALID_ARG);
			return my_ret;
		}
	}

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
		*my_ret = bcon_return_error(BVSP_ERR_DECODE_FAILED);
                return my_ret;
        }

	if(bcon_config->lock != NULL) {
        	int lck_ret = bcon_cfg_check_lockfile(1, bcon_config->lock);
	        if (lck_ret < 0) {
			*my_ret = bcon_return_error(BVSP_ERR_TIMEOUT);
                	return my_ret;
        	}
	        else if (lck_ret) {
			*my_ret = bcon_return_error(BVSP_ERR_TIMEOUT);
                	return my_ret;
        	}

        	if (bcon_cfg_write_lockfile(bcon_config->lock, &bcon_config->lock_fd) != 0) {
			*my_ret = bcon_return_error(BVSP_ERR_INTERNAL_FAILURE);
                	return my_ret;
        	}
	}

	if (stat(bcon_config->db_path, &cfgstat)) {
		if (errno == ENOENT) {
			if (!(bvsp_cfg_fp = fopen(bcon_config->db_path, "w"))) {
				if(bcon_config->lock != NULL)
					bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

				*my_ret = bcon_return_syserror(BVSP_ERR_OPEN_FAILED);
				return my_ret;
			}
			fclose(bvsp_cfg_fp);
			bvsp_cfg_fp = NULL; 
		} else {
			if(bcon_config->lock != NULL)
				bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

			*my_ret = bcon_return_syserror(BVSP_ERR_FAILED);
			return my_ret;
		}
	}

	if(action != SAVE_WITHOUT_TRIGGER) {
		*my_ret = bcon_do_trigger_oldvalues(TRIGGER_TYPE_SET, TRIGGER_PRE, key, val, num, NULL);
	}

	char * (*oldvalues)[] = NULL;
	if(action != TRIGGER_WITHOUT_SAVE) {
		if(bcon_config->trans_count != -1) {
			int idx;
			for(idx=0; idx<num; ++idx) {
				char hash_key[2048];
				sprintf(hash_key, "tran(%d):", bcon_config->trans_count);
				strcat(hash_key, key[idx]);
				bcon_trans_backup_nodes_db(bcon_handle, hash_key, key[idx], TRANS_TYPE_SET);
			}
		}
		ret = bcon_xml_nset_list((xmlDocPtr)bcon_config->db_cfg, (const xmlChar **)key, val, len, num, &oldvalues);
		bcon_save_config(bcon_config);
	}

	if(bcon_config->lock != NULL)
		bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

	char *onevalue = (oldvalues == NULL ? NULL : (*oldvalues)[0]);
	if(action != SAVE_WITHOUT_TRIGGER) {
		*my_ret = bcon_do_trigger_oldvalues((onevalue != NULL ? TRIGGER_TYPE_SET : TRIGGER_TYPE_CREATE), TRIGGER_POST, key, val, num, oldvalues);
	}

	bcon_free_oldvalues(oldvalues, num);

	if(*my_ret != BVSP_ERR_SUCCESS) {
		if(ret != NULL) {
			*ret = *my_ret;
		}
		else {
			ret = my_ret;

			my_ret = NULL;
		}
	}

	free(my_ret);

	return ret;
}


/* bcon_set_multiple_db_va_list

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_va_list(int bcon_handle, int num, va_list ap)
{
	return bcon_set_multiple_db_va_list_by_action(bcon_handle, SAVE_WITH_TRIGGER, num, ap);
}


/* bcon_set_multiple_db_va_list_no_trigger

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_va_list_no_trigger(int bcon_handle, int num, va_list ap)
{
	return bcon_set_multiple_db_va_list_by_action(bcon_handle, SAVE_WITHOUT_TRIGGER, num, ap);
}


/* bcon_set_multiple_db_va_list_no_save

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_va_list_no_save(int bcon_handle, int num, va_list ap)
{
	return bcon_set_multiple_db_va_list_by_action(bcon_handle, TRIGGER_WITHOUT_SAVE, num, ap);
}


/* bcon_set_multiple_db_va_list_by_action

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_va_list_by_action(int bcon_handle, int action, int num, va_list ap)
{
	bcon_init_success();

	int *ret = NULL;
	FILE *bvsp_cfg_fp;
	struct  stat cfgstat;
	int *my_ret = malloc(2*sizeof(int));
	*my_ret = BVSP_ERR_SUCCESS;
	*(my_ret+1) = 0;

	const xmlChar *key;
	const void *val;
	int len;
	int current_index = 0;
                
        char **keys = NULL;
	char **vals = NULL;
	int *lens = NULL;
        keys = (char **)malloc(sizeof(char *)*num);
        vals = (char **)malloc(sizeof(char *)*num);
        lens = (int *)malloc(sizeof(int *)*num);
	while(current_index < num) {
		key = va_arg(ap, const xmlChar *);
		val = va_arg(ap, const void *);
		len = va_arg(ap, int);

		if (!key || !val) {
			break;
		}

		if (key && !strlen((char *)key)) {
			va_end(ap);

			*my_ret = bcon_return_error(BVSP_ERR_INVALID_ARG);
			return my_ret;
		}

                keys[current_index] = (char *)key;
                vals[current_index] = (char *)val;
                lens[current_index] = len;

		++current_index;
	}
	num = current_index;

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
		free(keys);
		free(vals);
		free(lens);

		*my_ret = bcon_return_error(BVSP_ERR_DECODE_FAILED);
                return my_ret;
        }

	if(bcon_config->lock != NULL) {
        	int lck_ret = bcon_cfg_check_lockfile(1, bcon_config->lock);
	        if (lck_ret < 0) {
			free(keys);
			free(vals);
			free(lens);

			*my_ret = bcon_return_error(BVSP_ERR_TIMEOUT);
                	return my_ret;
        	}
	        else if (lck_ret) {
			free(keys);
			free(vals);
			free(lens);

			*my_ret = bcon_return_error(BVSP_ERR_TIMEOUT);
                	return my_ret;
        	}

        	if (bcon_cfg_write_lockfile(bcon_config->lock, &bcon_config->lock_fd) != 0) {
			free(keys);
			free(vals);
			free(lens);

			*my_ret = bcon_return_error(BVSP_ERR_INTERNAL_FAILURE);
                	return my_ret;
        	}
	}

	if (stat(bcon_config->db_path, &cfgstat)) {
		if (errno == ENOENT) {
			if (!(bvsp_cfg_fp = fopen(bcon_config->db_path, "w"))) {
				if(bcon_config->lock != NULL)
					bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

				free(keys);
				free(vals);
				free(lens);

				*my_ret = bcon_return_syserror(BVSP_ERR_OPEN_FAILED);
				return my_ret;
			}
			fclose(bvsp_cfg_fp);
			bvsp_cfg_fp = NULL; 
		} else {
			if(bcon_config->lock != NULL)
				bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

			free(keys);
			free(vals);
			free(lens);

			*my_ret = bcon_return_syserror(BVSP_ERR_FAILED);
			return my_ret;
		}
	}

	if(action != SAVE_WITHOUT_TRIGGER) {
		*my_ret = bcon_do_trigger_oldvalues(TRIGGER_TYPE_SET, TRIGGER_PRE, (const char **)keys, (const char **)vals, num, NULL);
	}

	char * (*oldvalues)[] = NULL;
	if(action != TRIGGER_WITHOUT_SAVE) {
		if(bcon_config->trans_count != -1) {
			int idx;
			for(idx=0; idx<num; ++idx) {
				char hash_key[2048];
				sprintf(hash_key, "tran(%d):", bcon_config->trans_count);
				strcat(hash_key, keys[idx]);
				bcon_trans_backup_nodes_db(bcon_handle, hash_key, keys[idx], TRANS_TYPE_SET);
			}
		}
		ret = bcon_xml_nset_list((xmlDocPtr)bcon_config->db_cfg, (const xmlChar **)keys, (const char **)vals, lens, num, &oldvalues);
		bcon_save_config(bcon_config);
	}

	if(bcon_config->lock != NULL)
		bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

	char *onevalue = (oldvalues == NULL ? NULL : (*oldvalues)[0]);
	if(action != SAVE_WITHOUT_TRIGGER) {
		*my_ret = bcon_do_trigger_oldvalues((onevalue != NULL ? TRIGGER_TYPE_SET : TRIGGER_TYPE_CREATE), TRIGGER_POST, (const char **)keys, (const char **)vals, num, oldvalues);
	}

	free(keys);
	free(vals);
	free(lens);

	if(*my_ret != BVSP_ERR_SUCCESS) {
		if(ret != NULL) {
			*ret = *my_ret;
		}
		else {
			ret = my_ret;

			my_ret = NULL;
		}
	}

	free(my_ret);

	return ret;
}


/* bcon_set_multiple_db_va_args

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_va_args(int bcon_handle, int num, ...)
{
	bcon_init_success();

	int *ret;
	va_list ap; 

	va_start(ap, num);
	ret = bcon_set_multiple_db_va_list_by_action(bcon_handle, SAVE_WITH_TRIGGER, num, ap);
	va_end(ap);

	return ret;
}


/* bcon_set_multiple_db_va_args_no_trigger

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_va_args_no_trigger(int bcon_handle, int num, ...)
{
	bcon_init_success();

	int *ret;
	va_list ap; 

	va_start(ap, num);
	ret = bcon_set_multiple_db_va_list_by_action(bcon_handle, SAVE_WITHOUT_TRIGGER, num, ap);
	va_end(ap);

	return ret;
}


/* bcon_set_multiple_db_va_args_no_save

   set a key to a value in the currently selected config
*/
int *bcon_set_multiple_db_va_args_no_save(int bcon_handle, int num, ...)
{
	bcon_init_success();

	int *ret;
	va_list ap; 

	va_start(ap, num);
	ret = bcon_set_multiple_db_va_list_by_action(bcon_handle, TRIGGER_WITHOUT_SAVE, num, ap);
	va_end(ap);

	return ret;
}


int bcon_set_int(const char *key, int i)
{
	bcon_init_success();

	char	buf[32];
	snprintf(buf, sizeof (buf), "%d", i);
	return bcon_set(key, buf, strlen(buf));
}


int bcon_set_int_db(int bcon_handle, const char *key, int i)
{
	bcon_init_success();

	char	buf[32];
	snprintf(buf, sizeof (buf), "%d", i);
	return bcon_set_db(bcon_handle, key, buf, strlen(buf));
}


int bcon_set_double(const char *key, double d)
{
	bcon_init_success();

	char buf[32];
	snprintf(buf, sizeof (buf), "%f", d);
	return bcon_set(key, buf, strlen(buf));
}


int bcon_set_double_db(int bcon_handle, const char *key, double d)
{
	bcon_init_success();

	char buf[32];
	snprintf(buf, sizeof (buf), "%f", d);
	return bcon_set_db(bcon_handle, key, buf, strlen(buf));
}


int bcon_set_str(const char *key, const char *str)
{
	bcon_init_success();

	return bcon_set(key, str, strlen(str));
}


int bcon_set_str_db(int bcon_handle, const char *key, const char *str)
{
	bcon_init_success();

	return bcon_set_db(bcon_handle, key, str, strlen(str));
}


int bcon_unset(const char *key)
{
	return bcon_unset_by_action(SAVE_WITH_TRIGGER, key);
}


int bcon_unset_no_save(const char *key)
{
	return bcon_unset_by_action(TRIGGER_WITHOUT_SAVE, key);
}


int bcon_unset_no_trigger(const char *key)
{
	return bcon_unset_by_action(SAVE_WITHOUT_TRIGGER, key);
}


int bcon_unset_by_action(int action, const char *key)
{
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	int ret = bcon_unset_db_by_action(default_bcon_handle, action, key);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}


int bcon_unset_db(int bcon_handle, const char *key)
{
	return bcon_unset_db_by_action(bcon_handle, SAVE_WITH_TRIGGER, key);
}


int bcon_unset_db_no_save(int bcon_handle, const char *key)
{
	return bcon_unset_db_by_action(bcon_handle, TRIGGER_WITHOUT_SAVE, key);
}


int bcon_unset_db_no_trigger(int bcon_handle, const char *key)
{
	return bcon_unset_db_by_action(bcon_handle, SAVE_WITHOUT_TRIGGER, key);
}


int bcon_unset_db_by_action(int bcon_handle, int action, const char *key)
{
	bcon_init_success();

	int ret_val = BVSP_ERR_SUCCESS;

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);

                return -1;
        }

	if(bcon_config->lock != NULL) {
        	int ret = bcon_cfg_check_lockfile(1, bcon_config->lock);
	        if (ret < 0) {
        	        return bcon_return_error(BVSP_ERR_TIMEOUT);
        	}
	        else if (ret) {
                	return bcon_return_error(BVSP_ERR_TIMEOUT);
        	}

        	if (bcon_cfg_write_lockfile(bcon_config->lock, &bcon_config->lock_fd) != 0) {
                	return -1;      /* It sets bvsp_errno */
        	}
	}

	char get_buf[2048];
	get_buf[0] = '\0';
	int ret = bcon_xml_get((xmlDocPtr)bcon_config->db_cfg, (const xmlChar *)key, get_buf, 2047);

	if(action != SAVE_WITHOUT_TRIGGER)
		ret_val = bcon_do_trigger(TRIGGER_TYPE_UNSET, TRIGGER_PRE, key, (ret > 0 ? get_buf : NULL), (ret > 0 ? get_buf : NULL));

	if(action != TRIGGER_WITHOUT_SAVE) {
		if (bcon_xml_unset((xmlDocPtr)bcon_config->db_cfg, (xmlChar *)key) < 0) {
			bcon_set_error(BVSP_ERR_FAILED);

			if(bcon_config->lock != NULL)
				bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

			return -1;
		}

		bcon_save_config(bcon_config);
	}

	if(bcon_config->lock != NULL)
		bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

	if(action != SAVE_WITHOUT_TRIGGER)
		ret_val = bcon_do_trigger(TRIGGER_TYPE_UNSET, TRIGGER_POST, key, (ret > 0 ? get_buf : NULL), (ret > 0 ? get_buf : NULL));

	return ret_val;
}


TRIGGER_MATCH *get_actions(int bcon_handle, int type, int op, char *trigger_key, char *trigger_match_key, TRIGGER_KEY_VALUE *key_values, TRIGGER_MATCH *match)
{
	char trigger_path[2048]; 
	char *trg_type;
	int trg_len;
	int trg_attr_len;
	char attr_key[2048];
	char attr_exec[2048];
	char buf[256];
	int length;
	int trigger_index;
	TRIGGER_MATCH *trigger_match = match;

	if(match == NULL || match->actions == NULL) {
		strcpy(trigger_path, trigger_key);
		int count = bcon_get_count_db_by_type(bcon_handle, DB_TRIGGER_TYPE, trigger_path); 
		if(count > 0) {
			trg_len = strlen(trigger_path) - 1;
			trg_type = (char *)BCON_TRIGGER_PATH_TRIGGER_TYPES[type];
			trigger_path[trg_len] = '\0';
			strcat(trigger_path, trg_type);
			trg_len = strlen(trigger_path);
			for(trigger_index=0; trigger_index<count; ++trigger_index) {
				trigger_path[trg_len] = '\0';
				sprintf(buf, "%d", trigger_index+1);
				strcat(trigger_path, buf);
				trg_attr_len = strlen(trigger_path);
				strcat(trigger_path, BCON_TRIGGER_PATH_ATTR_KEY);
				attr_key[0] = '\0';
				length = bcon_get_db_by_type(bcon_handle, DB_TRIGGER_TYPE, trigger_path, attr_key, sizeof(attr_key));
				if (length < 0) {
					/* announce that we're ignoring
					 * errors instead of doing it
					 * quietly to satisfy the compiler
					 * that we know what we're doing
					 * even though we don't.
					 */
				}
				trigger_path[trg_attr_len] = '\0';
				strcat(trigger_path, BCON_TRIGGER_PATH_ATTR_EXEC);
				attr_exec[0] = '\0';
				length = bcon_get_db_by_type(bcon_handle, DB_TRIGGER_TYPE, trigger_path, attr_exec, sizeof(attr_exec));
				if (length < 0) {
					/* announce that we're ignoring
					 * errors instead of doing it
					 * quietly to satisfy the compiler
					 * that we know what we're doing
					 * even though we don't.
					 */
				}

				if(strlen(attr_key) && strlen(attr_exec)) {
					if(trigger_match == NULL) {
						trigger_match = (TRIGGER_MATCH *)malloc(sizeof(TRIGGER_MATCH));
						strcpy(trigger_match->trigger_path, trigger_key);

						trigger_match->numb_of_key_values = 0;
						trigger_match->numb_of_actions = 0;
						trigger_match->key_values = NULL;
						trigger_match->actions = NULL;

						trigger_match->match_key = strdup(trigger_match_key); 
						trigger_match->op = op; 
						trigger_match->type = type; 
					}

					trigger_match->actions = (TRIGGER_ACTION **)realloc(trigger_match->actions, (size_t)(trigger_match->numb_of_actions+1)*sizeof(TRIGGER_ACTION *));
					trigger_match->actions[trigger_match->numb_of_actions] = (TRIGGER_ACTION *)malloc(sizeof(TRIGGER_ACTION));
					strcpy(trigger_match->actions[trigger_match->numb_of_actions]->exec_key, attr_key);
					strcpy(trigger_match->actions[trigger_match->numb_of_actions]->exec_cmd, attr_exec);
					++trigger_match->numb_of_actions;
				}
			}
		}
	}

	if(trigger_match != NULL) {
		trigger_match->key_values = (TRIGGER_KEY_VALUE **)realloc(trigger_match->key_values, (size_t)(trigger_match->numb_of_key_values+1)*sizeof(TRIGGER_KEY_VALUE *));						
		trigger_match->key_values[trigger_match->numb_of_key_values] = key_values;
		++trigger_match->numb_of_key_values;
	}

	return trigger_match;
}


int bcon_do_trigger_db_valist(int bcon_handle, int type, int op, ...) {
	int ret = BVSP_ERR_SUCCESS;

        va_list ap;
        char *key;
        void *val;


        va_start(ap, op);
        while(1) {
                key = va_arg(ap, char *);
                val = va_arg(ap, void *);

                if (!key || !val) {
                        break;
                }

                if (key && !strlen((char *)key)) {
                        ret = BVSP_ERR_INVALID_ARG;

			break;
                }
        }

        va_end(ap);

	return ret;
}


int do_exec(int bcon_handle, int op, int type, char *exec_key, char *exec_cmd, TRIGGER_KEY_VALUE **key_values, int count, char *trigger_path)
{
	int ret = BVSP_ERR_SUCCESS;
	int status;


	int index = 0;
	int key_index = 0;
	char **param_values = malloc((count+5)*sizeof(char *));
	param_values[0] = exec_cmd;
	param_values[1] = exec_key;
	param_values[2] = (char *)TRIGGER_OP_STR[op];
	param_values[3] = (char *)TRIGGER_TYPE_STR[type];
	for(index=4; index<count+4; ++index) {
		int str_len = strlen(key_values[key_index]->trigger_key) + 1;
		if(key_values[key_index]->value != NULL) {
			str_len += strlen(key_values[key_index]->value) + 1;
			if(key_values[key_index]->oldvalue != NULL)
				str_len += strlen(key_values[key_index]->oldvalue) + 1;
		}
		param_values[index] = malloc(str_len);
		strcpy(param_values[index], key_values[key_index]->trigger_key);
		if(key_values[key_index]->value != NULL) {
			strcat(param_values[index], ",");
			strcat(param_values[index], key_values[key_index]->value);
			if(key_values[key_index]->oldvalue != NULL) {
				strcat(param_values[index], ",");
				strcat(param_values[index], key_values[key_index]->oldvalue);
			}
		}
		++key_index;
	}
	param_values[index] = NULL;

	int str_pipes[2];
	if(pipe(str_pipes) != 0) {
		str_pipes[0] = -1;
		str_pipes[1] = -1;
	}
	int pipein = str_pipes[0];
	int pipeout = str_pipes[1];

	pid_t my_pid;

	my_pid = fork();
	if(my_pid < 0) {
		ret = BVSP_ERR_CONFIG_FAILED;
	} else if(my_pid == 0) {
		//int tmpstdin[2];
		//tmpstdin[0] = open("/dev/null", O_WRONLY | O_NOCTTY);
		//dup2(tmpstdin[0], STDERR_FILENO);
		//dup2(tmpstdin[0], STDOUT_FILENO);

		close(pipein);
		dup2(pipeout, STDOUT_FILENO);
		dup2(pipeout, STDERR_FILENO);
		close(pipeout);
		execvp(exec_cmd, param_values);

		_exit(1);
	}
	close(pipeout);

	char line[2048];
	char *buf = malloc(sizeof(char)*2048);
	int  buf_used = 0;
	int  buf_alloc = sizeof(char)*2048;
	char mode = 'r';
	FILE *readpipe = fdopen(pipein, &mode);

	int num_read = 0;
	*buf = '\0';
	while(!feof(readpipe)){
		if((num_read = fread(&line, sizeof(char), 2048, readpipe)) >= 1){
			if(buf_used+num_read > buf_alloc) {
				buf_alloc += sizeof(char)*2048;
				buf = realloc(buf, (size_t)(buf_alloc));
			}
			strncat(buf, line, num_read);
			buf_used += num_read;
		}
	}
    	fflush(stdout);
	fclose(readpipe);

	BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
	if(bcon_config != NULL) {
		free(bcon_config->last_output_buffer);
		free(bcon_config->last_output_buffer_error);
		bcon_config->last_output_buffer_error = NULL;

		bcon_config->last_output_buffer = buf;
	}

	close(pipein);

	wait(&status);

	//printf("  Status:       %d\n", status);
	//printf("  WIFSIGNALED:  %d\n", WIFSIGNALED(status));
	//printf("  WIFEXITED:    %d\n", WIFEXITED(status));
	//printf("  WEXITSTATUS:  %d\n", WEXITSTATUS(status));
	//printf("  WTERMSIG:     %d\n", WTERMSIG(status));

	//waitpid(my_pid, &status, 0);
	int exit_ret = WEXITSTATUS(status);
	if(exit_ret != 0)
		ret = BVSP_ERR_CONFIG_FAILED;

	if(bcon_config != NULL) {
		bcon_config->last_output_error_code = exit_ret;

		if(exit_ret != 0) {
			char *str_ptr = strstr(trigger_path, "/post_triggers");
			if(str_ptr == NULL) {
				str_ptr = strstr(trigger_path, "/pre_triggers");
			}
			if(str_ptr != NULL) {
				char error_code_str[32];
				char error_path[2048];
				strncpy(error_path, trigger_path, (int)(str_ptr-trigger_path));
				error_path[(int)(str_ptr-trigger_path) + 1] = '\0';
				strcat(error_path, "/error_codes/error[@code='");
				int err_end = strlen(error_path);
				sprintf(error_code_str, "%d", exit_ret);
				strcat(error_path, error_code_str);
				strcat(error_path, "']");
				char err_str[2048];
				err_str[0] = '\0';
				int err_len = bcon_get_db_by_type(bcon_handle, DB_TRIGGER_TYPE, error_path, err_str, sizeof(err_str));
				if(err_len <= 0) {
					error_path[err_end] = '\0';
					strcat(error_path, "*']");
					err_str[0] = '\0';
					err_len = bcon_get_db_by_type(bcon_handle, DB_TRIGGER_TYPE, error_path, err_str, sizeof(err_str));
				}
				if(err_len > 0) {
					bcon_config->last_output_buffer_error = strdup(err_str);	
				}
			}
		}
	}

        for(index=4; index<count+4; ++index) {
                free(param_values[index]);
        }
	free(param_values);


	return ret;
}


int bcon_do_trigger_db(int bcon_handle, int type, int op, TRIGGER_KEY_VALUE **key_values, int numb_of_values) {
	int ret = BVSP_ERR_SUCCESS;
	char trigger_path[2048]; 
	char trigger_match_key[2048];
	char trigger_scratch_key[2048];
	int numb_of_matches = 0;
	TRIGGER_MATCH **trigger_matches = NULL;

	int match_index;
	int child_level = 0;
	int key_index;

	/* only do 1 level down for wildcard now */
	while(child_level<2) {
		for(key_index=0; key_index<numb_of_values; ++key_index) {
			trigger_path[0] = '\0';
			strcat(trigger_path, BCON_TRIGGER_PATH_WITH_NAME);
			int key_len = strlen(key_values[0]->trigger_key);
			char *chr_ptr = strchr(key_values[0]->trigger_key, '[');
			if(chr_ptr != NULL)
				key_len = (int)(chr_ptr-key_values[0]->trigger_key);
			strncpy(trigger_scratch_key, key_values[0]->trigger_key, key_len);
			trigger_scratch_key[key_len] = '\0';
			strcpy(trigger_match_key, trigger_scratch_key);
			if(child_level > 0) {
				char *child_pos = strrchr(trigger_scratch_key, '/');
				if(child_pos != NULL) {
					int pos_int = (int)(child_pos-trigger_scratch_key+1);
					strncpy(trigger_match_key, trigger_scratch_key, pos_int);
					trigger_match_key[pos_int] = '*';
					trigger_match_key[pos_int+1] = '\0';
				} else {
					continue;
				}
			}
			strcat(trigger_path, trigger_match_key);
			if(op == TRIGGER_PRE)
				strcat(trigger_path, BCON_TRIGGER_PATH_END_NAME_WITH_PRE_ASTRK);
			else
				strcat(trigger_path, BCON_TRIGGER_PATH_END_NAME_WITH_POST_ASTRK);

			for(match_index=0; match_index<numb_of_matches; ++match_index) {
				if(!strcmp(trigger_matches[match_index]->match_key, trigger_match_key)) {
					get_actions(bcon_handle, type, op, trigger_path, trigger_match_key, key_values[key_index], trigger_matches[match_index]);				

					break;
				}
			}

			if(match_index >= numb_of_matches) {
				TRIGGER_MATCH *match_found = get_actions(bcon_handle, type, op, trigger_path, trigger_match_key, key_values[key_index], NULL);				
				if(match_found) {
					trigger_matches = (TRIGGER_MATCH **)realloc(trigger_matches, sizeof(TRIGGER_MATCH *)*(numb_of_matches+1));
					trigger_matches[numb_of_matches] = match_found;

					++numb_of_matches;
				}
			}
		} 

		++child_level;
	}

	int action_index;
	for(match_index=0; match_index<numb_of_matches; ++match_index) {
		for(action_index=0; action_index<trigger_matches[match_index]->numb_of_actions; ++action_index) {
			ret = do_exec(bcon_handle, op, type, trigger_matches[match_index]->actions[action_index]->exec_key, trigger_matches[match_index]->actions[action_index]->exec_cmd, trigger_matches[match_index]->key_values, trigger_matches[match_index]->numb_of_key_values, trigger_matches[match_index]->trigger_path);

			free(trigger_matches[match_index]->actions[action_index]);
		}

		free(trigger_matches[match_index]->actions);
		free(trigger_matches[match_index]->key_values);
		free(trigger_matches[match_index]->match_key);
		free(trigger_matches[match_index]);
	}

	return ret;
}


int bcon_do_trigger_values(int type, int op, TRIGGER_KEY_VALUE **key_values, int numb_of_values) {
	int ret = BVSP_ERR_SUCCESS;

	int free_default_bcon_handle = 0;

	bcon_init_success();

        if(!default_bcon_handle) {
                bcon_init();
                free_default_bcon_handle = 1;
        }

	ret = bcon_do_trigger_db(default_bcon_handle, type, op, key_values, numb_of_values);

        if(free_default_bcon_handle)
                bcon_cleanup();

	return ret;
}

int bcon_do_trigger_oldvalues(int type, int op, const char **trigger_key, const char **trigger_value, int len, char* (*trigger_oldvalues)[]) {
	int index;
	TRIGGER_KEY_VALUE **key_values;
	key_values = (TRIGGER_KEY_VALUE **)malloc(sizeof(TRIGGER_KEY_VALUE *)*len);
	for(index=0; index<len; ++index) {
		key_values[index] = (TRIGGER_KEY_VALUE *)malloc(sizeof(TRIGGER_KEY_VALUE));
		key_values[index]->trigger_key = (char *)trigger_key[index];
		key_values[index]->value = (char *)trigger_value[index];
		key_values[index]->oldvalue = (trigger_oldvalues == NULL ? NULL : (*trigger_oldvalues)[index]);
	}

	int ret = bcon_do_trigger_values(type, op, key_values, len);

	for(index=0; index<len; ++index) {
		free(key_values[index]);
	}
	free(key_values);

	return ret;
}

int bcon_do_trigger(int type, int op, const char *trigger_key, const char *trigger_value, char *trigger_oldvalue) {
	TRIGGER_KEY_VALUE key_value;
	TRIGGER_KEY_VALUE *key_values[1];
	key_value.trigger_key = (char *)trigger_key;
	key_value.value = (char *)trigger_value;
	key_value.oldvalue = trigger_oldvalue;
	key_values[0] = &key_value;

	return bcon_do_trigger_values(type, op, key_values, 1);
}


void bcon_node_free(void *node_copy) {
	if(node_copy != NULL)
		bcon_xml_free_copy((xmlXPathObjectPtr)node_copy);
}


int bcon_trans_backup_nodes(const char *hash_key, const char *node_key, int trans_action)
{
        int ret;
        int free_default_bcon_handle = 0;

        bcon_init_success();

        if(!default_bcon_handle) {
                bcon_init();
                free_default_bcon_handle = 1;
        }

        ret = bcon_trans_backup_nodes_db(default_bcon_handle, hash_key, node_key, trans_action);

        if(free_default_bcon_handle)
                bcon_cleanup();

        return ret;
}


int bcon_trans_backup_nodes_db(int bcon_handle, const char *hash_key, const char *node_key, int trans_action)
{
	bcon_init_success();

	int ret = BVSP_ERR_SUCCESS;
        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);

                return -1;
        }

//printf("node_copy = %s\n", (char *)node_key);
	void *node_copy = bcon_xml_get_copy(bcon_config->db_cfg, (const xmlChar *)node_key, NULL);
	if(node_copy != NULL || trans_action == TRANS_TYPE_SET) {
		TRANSACTION_TYPE *trans_type = (TRANSACTION_TYPE *)malloc(sizeof(TRANSACTION_TYPE));
		trans_type->hash_key = strdup((char *)hash_key);
		trans_type->node_key = strdup((char *)node_key);
		char remove_key[2048];
		remove_key[0] = '\0';
		if(trans_action == TRANS_TYPE_SET) {
/*
			int cnt = bcon_get_count_db(bcon_handle, node_key);
			if(cnt <= 0) {
				strcpy(remove_key, (char *)node_key);
			}
			char backup_key[2048];
			strcpy(backup_key, node_key);
			int str_idx = (int)strrchr(backup_key, '/') - (int)backup_key;
			if(str_idx >= 0) {
				backup_key[str_idx] = '\0';
				cnt = bcon_get_count_db(bcon_handle, backup_key);
				if(cnt <= 0) {
					strcpy(remove_key, backup_key);
				} else {
					strcpy(remove_key, (char *)node_key);
				}
			}
*/

			strcpy(remove_key, (char *)node_key);
			int cnt = bcon_get_count_db(bcon_handle, node_key);
			if(cnt <= 0) {
				int str_idx = -1;
				char *end_ptr = NULL;
				char backup_key[2048];
				strcpy(backup_key, node_key);
				while(1) {
					str_idx = strlen(backup_key)-1;
					if(str_idx >= 0 && backup_key[str_idx] == ']') {
						end_ptr = strrchr(backup_key, '[');
						if(end_ptr) {
							*end_ptr = '\0';
							end_ptr = strrchr(backup_key, '/');
						}
					} else {
						end_ptr = strrchr(backup_key, '/');
					}
					if(end_ptr) {
						*end_ptr = '\0';
						cnt = bcon_get_count_db(bcon_handle, backup_key);
						if(cnt <= 0) {
							strcpy(remove_key, backup_key);
						} else {
							break;
						}
					} else{
						break;
					}
				}
			}
		}
		if(strlen(remove_key) > 0)
			trans_type->remove_key = strdup(remove_key);
		else
			trans_type->remove_key = NULL;
		trans_type->trans_action = trans_action;
		trans_type->data = node_copy;
		unsigned hashcode = hash_HashFunction((const u_char *)trans_type->hash_key, strlen(trans_type->hash_key));
		boolean not_inserted = hash_Insert(bcon_config->tr_hash_table, hashcode, nullcmp, (hash_datum *)trans_type->hash_key, trans_type);
		if(not_inserted) {
			free(trans_type->hash_key);
			free(trans_type->node_key);
			free(trans_type->remove_key);
			bcon_node_free(trans_type->data);
			free(trans_type);

			ret = BVSP_ERR_DECODE_FAILED;
		} else if(bcon_config->trans_count != -1) {
			++bcon_config->trans_count;
			bcon_config->trans_data = (char **)realloc(bcon_config->trans_data, sizeof(TRANSACTION_TYPE *)*bcon_config->trans_count);
			bcon_config->trans_data[bcon_config->trans_count-1] = strdup(hash_key);
		}
	} else {
		ret = BVSP_ERR_DECODE_FAILED;
	}

	return ret;
}


int bcon_trans_restore_nodes(const char *hash_key)
{
        int ret;
        int free_default_bcon_handle = 0;

        bcon_init_success();

        if(!default_bcon_handle) {
                bcon_init();
                free_default_bcon_handle = 1;
        }

        ret = bcon_trans_restore_nodes_db(default_bcon_handle, hash_key);

        if(free_default_bcon_handle)
                bcon_cleanup();

        return ret;
}


int bcon_trans_restore_nodes_db(int bcon_handle, const char *hash_key)
{
	bcon_init_success();

	int ret = BVSP_ERR_SUCCESS;
        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);

                return -1;
        }

        if(bcon_config->lock != NULL) {
                int ret = bcon_cfg_check_lockfile(1, bcon_config->lock);
                if (ret < 0) {
                        return bcon_return_error(BVSP_ERR_TIMEOUT);
                }
                else if (ret) {
                        return bcon_return_error(BVSP_ERR_TIMEOUT);
                }

                if (bcon_cfg_write_lockfile(bcon_config->lock, &bcon_config->lock_fd) != 0) {
                        return -1;      /* It sets bvsp_errno */
                }
        }

	unsigned hashcode = hash_HashFunction((const u_char *)hash_key, strlen(hash_key));
        TRANSACTION_TYPE *trans_type = (TRANSACTION_TYPE *)hash_Lookup(bcon_config->tr_hash_table, hashcode, handletrcmp, (hash_datum *)hash_key);
	if(trans_type != NULL) {
		int str_idx = -1;
		char *end_ptr = NULL;
		char backup_key[2048];
		strcpy(backup_key, trans_type->node_key);
		str_idx = strlen(backup_key)-1;
		if(str_idx >= 0 && backup_key[str_idx] == ']') {
			end_ptr = strrchr(backup_key, '[');
			if(end_ptr) {
				*end_ptr = '\0';
				end_ptr = strrchr(backup_key, '/');
			}
		} else {
			end_ptr = strrchr(backup_key, '/');
		}
		if(end_ptr)
			*end_ptr = '\0';
		ret = bcon_set_node_from_copy_db(bcon_config->db_cfg, trans_type->data, backup_key);
		if(ret == BVSP_ERR_SUCCESS) {
			//trans_type->data = NULL;
		}
		//hash_Delete(bcon_config->tr_hash_table, hashcode, handletrcmp, (hash_datum *)hash_key, freetrptr);

		bcon_save_config(bcon_config);
	} else {
		ret = BVSP_ERR_DECODE_FAILED;
	}

        if(bcon_config->lock != NULL)
                bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

	return ret;
}


int bcon_trans_free_nodes(const char *hash_key)
{
        int ret;
        int free_default_bcon_handle = 0;

        bcon_init_success();

        if(!default_bcon_handle) {
                bcon_init();
                free_default_bcon_handle = 1;
        }

        ret = bcon_trans_free_nodes_db(default_bcon_handle, hash_key);

        if(free_default_bcon_handle)
                bcon_cleanup();

        return ret;
}


int bcon_trans_start( void )
{
        int ret;
        int free_default_bcon_handle = 0;

        bcon_init_success();

        if(!default_bcon_handle) {
                bcon_init();
                free_default_bcon_handle = 1;
        }

        ret = bcon_trans_start_db(default_bcon_handle);

        if(free_default_bcon_handle)
                bcon_cleanup();

        return ret;
}


int bcon_trans_start_db(int bcon_handle)
{
        bcon_init_success();

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);
                
                return BVSP_ERR_DECODE_FAILED;
        }

	bcon_config->trans_count = 0;

	++next_trans;

	return BVSP_ERR_SUCCESS;
}


int bcon_trans_end( void )
{
        int ret;
        int free_default_bcon_handle = 0;

        bcon_init_success();

        if(!default_bcon_handle) {
                bcon_init();
                free_default_bcon_handle = 1;
        }

        ret = bcon_trans_end_db(default_bcon_handle);

        if(free_default_bcon_handle)
                bcon_cleanup();

        return ret;
}


int bcon_trans_end_db(int bcon_handle)
{
        bcon_init_success();

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);
                
                return BVSP_ERR_DECODE_FAILED;
        }

	if(bcon_config->trans_data != NULL) {
		int idx;
		for(idx=0; idx<bcon_config->trans_count; ++idx) {
			free(bcon_config->trans_data[idx]);
		}
		free(bcon_config->trans_data);
		bcon_config->trans_data = NULL;
		bcon_config->trans_count = -1;
	}

	return BVSP_ERR_SUCCESS;
}


int bcon_trans_commit( void )
{
        int ret;
        int free_default_bcon_handle = 0;

        bcon_init_success();

        if(!default_bcon_handle) {
                bcon_init();
                free_default_bcon_handle = 1;
        }

        ret = bcon_trans_commit_db(default_bcon_handle);

        if(free_default_bcon_handle)
                bcon_cleanup();

        return ret;
}


int bcon_trans_commit_db(int bcon_handle)
{
	return bcon_trans_end_db(bcon_handle);
}


int bcon_trans_revert( void )
{
        int ret;
        int free_default_bcon_handle = 0;

        bcon_init_success();

        if(!default_bcon_handle) {
                bcon_init();
                free_default_bcon_handle = 1;
        }

        ret = bcon_trans_revert_db(default_bcon_handle);

        if(free_default_bcon_handle)
                bcon_cleanup();

        return ret;
}


int bcon_trans_revert_db(int bcon_handle)
{
        bcon_init_success();

	int ret = BVSP_ERR_SUCCESS;

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);
                
                return BVSP_ERR_DECODE_FAILED;
        }

        if(bcon_config->lock != NULL) {
                int lck_ret = bcon_cfg_check_lockfile(1, bcon_config->lock);
                if (lck_ret < 0) {
                        return bcon_return_error(BVSP_ERR_TIMEOUT);
                }
                else if (lck_ret) {
                        return bcon_return_error(BVSP_ERR_TIMEOUT);
                }

                if (bcon_cfg_write_lockfile(bcon_config->lock, &bcon_config->lock_fd) != 0) {
                        return -1;      /* It sets bvsp_errno */
                }
        }

	if(bcon_config->trans_data != NULL) {
		int idx;
		for(idx=bcon_config->trans_count-1; idx>=0; --idx) {
			char *hash_key = bcon_config->trans_data[idx];
			unsigned hashcode = hash_HashFunction((const u_char *)hash_key, strlen(hash_key));
        		TRANSACTION_TYPE *trans_type = (TRANSACTION_TYPE *)hash_Lookup(bcon_config->tr_hash_table, hashcode, handletrcmp, (hash_datum *)hash_key);
			if(trans_type != NULL) {
				if(trans_type->trans_action == TRANS_TYPE_SET) {
					if(trans_type->remove_key != NULL) {
//printf("unset key = %s\n", (char *)trans_type->remove_key);
						bcon_xml_unset((xmlDocPtr)bcon_config->db_cfg, (xmlChar *)trans_type->remove_key);
					}
				} else if(trans_type->trans_action == TRANS_TYPE_UNSET) {
				}
				if(trans_type->data != NULL) {
					int str_idx = -1;
					char *end_ptr = NULL;
					char backup_key[2048];
					strcpy(backup_key, trans_type->node_key);
					str_idx = strlen(backup_key)-1;
					if(str_idx >= 0 && backup_key[str_idx] == ']') {
						end_ptr = strrchr(backup_key, '[');
						if(end_ptr) {
							*end_ptr = '\0';
							end_ptr = strrchr(backup_key, '/');
						}
					} else {
						end_ptr = strrchr(backup_key, '/');
					}
					if(end_ptr)
						*end_ptr = '\0';
//printf("set node key = %s\n", backup_key);
					ret = bcon_set_node_from_copy_db(bcon_config->db_cfg, trans_type->data, backup_key);
                			if(ret == BVSP_ERR_SUCCESS) {
                        			trans_type->data = NULL;
                			}
				}
			}

			free(bcon_config->trans_data[idx]);
		}

		bcon_save_config(bcon_config);

		free(bcon_config->trans_data);
		bcon_config->trans_data = NULL;
		bcon_config->trans_count = -1;
	}

	if(bcon_config->lock != NULL)
		bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

	return ret;
}


int bcon_trans_free_nodes_db(int bcon_handle, const char *hash_key)
{
        bcon_init_success();

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);
                
                return BVSP_ERR_DECODE_FAILED;
        }

	unsigned hashcode = hash_HashFunction((const u_char *)hash_key, strlen(hash_key));
	hash_Delete(bcon_config->tr_hash_table, hashcode, handletrcmp, (hash_datum *)hash_key, freetrptr);

	return BVSP_ERR_SUCCESS;
}



/* bcon_get_node_copy

   get a copy of the nodes under the parent by key
*/
void * bcon_get_node_copy_db(BConCfgPtr db_cfg, const char *parent_key)
{
	return bcon_xml_get_copy(db_cfg, (const xmlChar *)parent_key, (const xmlChar *)"*");
}



/* bcon_set_node_from_copy

   set nodes under the parent by key from copy
*/
int bcon_set_node_from_copy_db(BConCfgPtr db_cfg, void *node_copy, const char *parent_key)
{
	return bcon_xml_set_from_copy((xmlDocPtr)db_cfg, node_copy,
					   (const xmlChar *)parent_key);
}

int bcon_free_backup_children(const char *parent_key) {
	int ret;
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	ret = bcon_free_backup_children_db(default_bcon_handle, parent_key);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}  

int bcon_free_backup_children_db(int bcon_handle, const char *parent_key) {
        bcon_init_success();

        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);
                
                return BVSP_ERR_DECODE_FAILED;
        }

	free(bcon_config->parent_key);
	bcon_node_free(bcon_config->child_nodes);

	bcon_config->parent_key = NULL;
	bcon_config->child_nodes = NULL;

	return BVSP_ERR_SUCCESS;
}

int bcon_backup_children(const char *parent_key) {
	int ret;
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	ret = bcon_backup_children_db(default_bcon_handle, parent_key);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}

int bcon_backup_children_db(int bcon_handle, const char *parent_key) {
	bcon_init_success();

	//int ret;
        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);

                return -1;
        }

	if(bcon_config->parent_key != NULL) {
		free(bcon_config->parent_key);
		bcon_config->parent_key = NULL;
	}

	if(bcon_config->child_nodes != NULL) {
		bcon_xml_free_copy(bcon_config->child_nodes);
		bcon_config->child_nodes = NULL;
	}

	char backup_key[2048];
	int len = strlen(parent_key);
	strcpy(backup_key, parent_key);
	if(backup_key[len-1] != '/') {
		strcat(backup_key, "/");
	}	
	bcon_config->child_nodes = bcon_get_node_copy_db(bcon_config->db_cfg, backup_key);
	if(bcon_config->child_nodes != NULL)
		bcon_config->parent_key = strdup(parent_key);

	return (bcon_config->child_nodes != NULL ? BVSP_ERR_SUCCESS : BVSP_ERR_DECODE_FAILED);
}

int bcon_restore_children(const char *parent_key) {
	int ret;
	int free_default_bcon_handle = 0;

	bcon_init_success();

	if(!default_bcon_handle) {
		bcon_init();
		free_default_bcon_handle = 1;
	}

	ret = bcon_restore_children_db(default_bcon_handle, parent_key);

	if(free_default_bcon_handle)
		bcon_cleanup();

	return ret;
}

int bcon_restore_children_db(int bcon_handle, const char *parent_key) {
	bcon_init_success();

	int ret;
        BCON_CONFIG *bcon_config = bcon_get_config(bcon_handle);
        if (!bcon_config || bcon_config->db_cfg == NULL) {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);

                return -1;
        }

        if(bcon_config->lock != NULL) {
                int ret = bcon_cfg_check_lockfile(1, bcon_config->lock);
                if (ret < 0) {
                        return bcon_return_error(BVSP_ERR_TIMEOUT);
                }
                else if (ret) {
                        return bcon_return_error(BVSP_ERR_TIMEOUT);
                }

                if (bcon_cfg_write_lockfile(bcon_config->lock, &bcon_config->lock_fd) != 0) {
                        return -1;      /* It sets bvsp_errno */
                }
        }

	if(bcon_config->child_nodes != NULL && bcon_config->parent_key != NULL) {
		char backup_key[2048];
		int len = strlen(parent_key);
		strcpy(backup_key, parent_key);
		if(backup_key[len-1] == '/') {
			backup_key[len-1] = '\0';
		}	
		ret = bcon_set_node_from_copy_db(bcon_config->db_cfg, bcon_config->child_nodes, backup_key);
		if(ret == BVSP_ERR_SUCCESS) {
			free(bcon_config->parent_key);

			bcon_config->parent_key = NULL;
			bcon_config->child_nodes = NULL;
		}

		bcon_save_config(bcon_config);
	} else {
                bcon_set_error(BVSP_ERR_DECODE_FAILED);

		ret = BVSP_ERR_DECODE_FAILED;
	}

        if(bcon_config->lock != NULL)
                bcon_cfg_remove_lockfile(bcon_config->lock, &bcon_config->lock_fd);

	return ret;
}
