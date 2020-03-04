#!/usr/bin/python -u

import os
import getopt
import sys
import traceback
import re
import socket
import datetime
import urllib
import urllib2
import subprocess
import sha
import string
import xml.dom.minidom
import time
import logging

from xml.dom.minidom import parse
from xml.dom import minidom
from time import time

LOG = logging.getLogger('deploy_tool')
console = logging.StreamHandler()
LOG.addHandler(console)

def dmvlicense():
    #Initialize variables
    licensingServer = 'https://www.iivipconnect.com/unleash'
    licenseConfig = '/interact/master/userLicense.cfg'
    userAuth = '/interact/master/userAuthentication.txt'
    uuid = 'aa538f9b-1952-45c2-b403-acc187f96f78'
    availableAction = 'api/licensing/available'

    # Get the available components for this user.
    url = "%s/%s?uuid=%s" % (licensingServer, availableAction, uuid)
    try:
        LOG.info('Contacting url: ' + url)
        urlfile = urllib.urlopen(url)

    except Exception:
        LOG.exception(url + ' not found.')
        sys.exit(1)

    # Parse the XML to get the features.
    try:
        xmldoc = minidom.parse(urlfile)

	"""
	Assuming this is file mymodule.py, then this string, being the
	first statement in the file, will become the "mymodule" module's
	docstring when the file is imported.
	"""

    except Exception:
        LOG.exception('Error parsing the XML document from ' + url)
        sys.exit(1)

    # Open the config file to write our license information.
    licenseConfigFile = None
    try:
        try:
            # Check if file already exists. If it does back it up.
            if os.path.exists(licenseConfig):
                os.rename(licenseConfig, "%s%s" % (licenseConfig, time()))

            # Open the config file for writing
            licenseConfigFile = open(licenseConfig,'w')

            #Get each of the feature elements.
            for fref in xmldoc.getElementsByTagName('feature'):
                name = fref.attributes["name"].value
                type = fref.attributes["type"].value
                cnt = fref.attributes["count"].value
                expiry = fref.attributes["expiry"].value

                #License all per-host features.
                if type == 'Per-Host':
                    licenseConfigFile.write("%s=Y\n" % name)

                #License all per-channel features.
                if type == 'Per-Channel':
                    licenseConfigFile.write("%s=1000\n" % name)

            #Change ownership of the files.
            doCmd("chown interact.operator " + licenseConfig)
            doCmd("chmod 766 " + licenseConfig)

        except Exception:
            LOG.exception('Error opening file: ' + licenseConfig + ' Make sure the directory exists and you have permissions to write to it.')
            sys.exit(1)

    finally:
        if licenseConfigFile:
            licenseConfigFile.close()

    #Open the userAuth file to write our user information.
    userAuthFile = None
    try:
        try:
            userAuthFile = open(userAuth,'w')
            userAuthFile.write("%s\n" % licensingServer)
            userAuthFile.write("%s\n" % uuid)

            doCmd("chown interact.operator %s" % userAuth)
            doCmd("chmod 766 %s" % userAuth)

        except Exception:
            LOG.exception('Error opening file: ' + userAuth + ' Make sure the directory exists and you have permissions to write to it.')

    finally:
        if userAuthFile:
            userAuthFile.close()

def subprocess_cmd(command):
    process = subprocess.Popen(command,stdout=subprocess.PIPE, shell=True)
    proc_stdout = process.communicate()[0].strip()
    return proc_stdout

def find_owner():
    bashCommand = "grep 'InstanceAdministrator=*' /etc/TimesTen/instance_info | sed 's/.*=//'"
    outputvar = subprocess_cmd(bashCommand)
    return outputvar

def doCmd(oscmd):
    try:
        now=datetime.datetime.now()
        LOG.info('%s %s %s' % (str(now),'deploy_tool>', oscmd))
        osrc=os.system(oscmd)
        if (osrc != 0):
            LOG.critical('Command: %s on %s returned: %s' % (oscmd,hostname,str(osrc)))
            sys.exit(1)
    except:
        LOG.exception('Command: %s generated an exception' % (oscmd))
        sys.exit(1)

def doAnyway(oscmd):
    try:
        now=datetime.datetime.now()
        LOG.info('%s %s %s' % (str(now),'deploy_tool>', oscmd))
        osrc=os.system(oscmd)
        if (osrc != 0):
            LOG.critical('Command: %s on %s returned: %s' % (oscmd,hostname,str(osrc)))
    except:
        LOG.exception('Command: %s generated an exception' % (oscmd))

def doPrep():
    if hostname==rtbserver:
        LOG.info("Preparing for RTB deployment")
        if os.path.isfile("/etc/init.d/collector"):
            doAnyway("/etc/init.d/collector stop")
        elif os.path.isfile("/usr/lib/systemd/system/collector.service"):
            doAnyway("/usr/bin/systemctl stop collector")

        if os.path.isfile("/etc/init.d/ladder"):
            doAnyway("/etc/init.d/ladder stop")
        
        if os.path.isfile("/etc/init.d/dmv"):
            doAnyway("/etc/init.d/dmv stop")
        elif os.path.isfile("/usr/lib/systemd/system/dmv.service"):
            doAnyway("/usr/bin/systemctl stop dmv")            
            
        if os.path.isfile("/opt/TimesTen/tt1122/bin/ttDaemonAdmin"):
	        doAnyway("/opt/TimesTen/tt1122/bin/ttDaemonAdmin -stop")

        if os.path.isfile("/interact/invigorate-db/TimesTen/tt1122/bin/setup.sh") & os.path.isfile("/etc/TimesTen/instance_info"):
            if find_owner()=='interact':
				doAnyway("su - interact -c '/interact/invigorate-db/TimesTen/tt1122/bin/setup.sh -uninstall -batch /interact/invigorate-db/conf/uninstall.ini'")
            else:
                doAnyway("/interact/invigorate-db/TimesTen/tt1122/bin/setup.sh -uninstall -batch /interact/invigorate-db/conf/uninstall.ini")
        elif os.path.isfile("/interact/invigorate-db/TimesTen/tt1122/bin/setup.sh"):
    		doAnyway("/interact/invigorate-db/TimesTen/tt1122/bin/setup.sh -uninstall -batch /interact/invigorate-db/conf/uninstall.ini")

        # stop iiTes if present
        if os.path.isfile("/etc/init.d/tes"):
            doAnyway("/etc/init.d/tes stop")
        elif os.path.isfile("/usr/lib/systemd/system/tes.service"):
            doAnyway("/usr/bin/systemctl stop tes")
            
        # stopping moap, cleaning shared memory, socket file, etc
        if os.path.isfile("/usr/lib/systemd/system/moap.service"):
            doAnyway("/usr/bin/systemctl stop moap")
        elif os.path.isfile("/etc/init.d/vipStart"):
            doAnyway("/etc/init.d/vipStart stop")

            
        if os.path.isfile("/interact/program/iiRemoveSharedMemory.sh"):
            doAnyway("sh /interact/program/iiRemoveSharedMemory.sh --force")

        # stopping wdm
        if os.path.isfile("/etc/init.d/wdm"):
            doAnyway("/etc/init.d/wdm stop")
        elif os.path.isfile("/usr/lib/systemd/system/wdm.service"):
            doAnyway("/usr/bin/systemctl stop wdm")

        # stopping vipSnmpd
        if os.path.isfile("/etc/init.d/vipSnmpd"):
            doAnyway("/etc/init.d/vipSnmpd stop")
        elif os.path.isfile("/usr/lib/systemd/system/vipSnmpd.service"):
            doAnyway("/usr/bin/systemctl stop vipSnmpd")

    if hostname==dbserver:
        LOG.info("Preparing for DB deployment")
        if os.path.isfile("/etc/init.d/collector"):
            doAnyway("/etc/init.d/collector stop")
        elif os.path.isfile("/usr/lib/systemd/system/collector.service"):
            doAnyway("/usr/bin/systemctl stop collector")
        doAnyway("/etc/init.d/ladder stop")
        doAnyway("/etc/init.d/drasctl stop")
        if os.path.isfile("/etc/init.d/dmv"):
            doAnyway("/etc/init.d/dmv stop")
        elif os.path.isfile("/usr/lib/systemd/system/dmv.service"):
            doAnyway("/usr/bin/systemctl stop dmv")            
        doAnyway("su - oracle -c '/home/oracle/log_purge.sh'")

    if hostname==webserver:
        LOG.info("Preparing for WEB deployment")
        # and this is hideous, but let's shut down some software
        doAnyway("/etc/init.d/tomcat stop")
        doAnyway("/etc/init.d/portal stop")
        if os.path.isfile("/etc/init.d/collector"):
            doAnyway("/etc/init.d/collector stop")
        elif os.path.isfile("/usr/lib/systemd/system/collector.service"):
            doAnyway("/usr/bin/systemctl stop collector")
        doAnyway("/etc/init.d/ladder stop")
        if os.path.isfile("/etc/init.d/dmv"):
            doAnyway("/etc/init.d/dmv stop")
        elif os.path.isfile("/usr/lib/systemd/system/dmv.service"):
            doAnyway("/usr/bin/systemctl stop dmv")            
        doAnyway("/etc/init.d/tes stop")
        doAnyway("/etc/init.d/legacy stop")
        doAnyway("/etc/init.d/ratesync stop")
        doAnyway("/etc/init.d/smpBroker stop")
        doAnyway("pkill -KILL ladder")
        doAnyway("pkill -KILL iiTes")
        doAnyway("pkill -KILL jsvc.exec")
        doAnyway("pkill -KILL -f /interact")
        doAnyway("pkill -KILL -f RATESYNC")

    # setup hosts file
    try:
        # Output standard info
        LOG.info("Generating hosts file")
        fileHandle = open( "/etc/hosts" , "w+" )
        fileHandle.write("# Do not remove the following line, or various programs\n")
        fileHandle.write("# that require network functionality will fail.\n")
        fileHandle.write("127.0.0.1       localhost.localdomain localhost\n")
        fileHandle.write("::1             localhost6.localdomain6 localhost6\n")
        fileHandle.write("\n")

        # Output host aliases
        for host, alias in ([rtbserver, "defaultrtb"], [dbserver, "defaultdb"], [webserver, "defaultweb"], [realizeserver, "defaultrealize"]):
            #loop through and lookup names for all hosts that
            #populated
            if host != None and host != "":
                try:
                    ip = socket.gethostbyname(host)
                    # name determined, write to hosts file
                    fileHandle.write(ip + " " + host + " " + alias + " " + alias + ".interact.nonreg\n")

                except:
                    LOG.exception('Unable to retrieve ip information for [ %s ]:' % (str(host)))
                    sys.exit(1)

    finally:
        fileHandle.close()

    # assume that one and only one InvigorateXXX.rpm is in /interact/packages

	"""
	Assuming this is file mymodule.py, then this string, being the
	first statement in the file, will become the "mymodule" module's
	docstring when the file is imported.
	"""

    LOG.info('removing old Interact packages')
    goodpacks=['xtt','xvm-tools','iiweb-sim','iiweb-test','iiweb-devel']
    nukelist = []
    packages=os.popen('rpm -qg Interact')
    for p in packages.readlines():
        nukelist.append(p)
        for g in goodpacks:
            if (p.find(g) > -1):
                LOG.info("skipped removal of %s" % (p))
                nukelist.remove(p)

    walkingdead=" ".join(nukelist)
    deathrow=walkingdead.replace("\n","")

    if (len(deathrow) > 0):
        removeify = "rpm -e --allmatches %s" % deathrow
        LOG.info('removing %s' % (deathrow))
        doAnyway(removeify)


    # smack the hell out of the /interact directory
    LOG.info("Cleaning /interact directory")
    pfiles = os.listdir('/interact')
    nukelist=[]
    for pf in pfiles:
        nukelist.append(pf)
        if (pf.find('.rpm') > -1):
            # preserve rpm files
            nukelist.remove(pf)

        if (pf.find('.ii') > -1):
            # preserve ii files
            nukelist.remove(pf)

        if (pf.find('xtt') > -1):
            # preserve xtt
            nukelist.remove(pf)

    for pf in nukelist:
        removeify = "rm -rf /interact/%s" % (pf)
        doCmd(removeify)


    # remove interact user
    LOG.info('Removing interact user')
    doAnyway("userdel interact")
    doAnyway("groupdel operator")


# script starts here and stuff
# determine the basics and parse args
def main():

    # some globals, because lazy programmers love globals
    global phase
    global webserver
    global rtbserver
    global dbserver
    global realizeserver
    global hostname
    global log

    phase="unset"
    webserver="unset"
    rtbserver="unset"
    realizeserver=None
    dbserver="unset"

    # set up logging

    logfile = logging.FileHandler('deploy.log')
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    logfile.setFormatter(formatter)
    LOG.addHandler(logfile)
    LOG.setLevel(logging.DEBUG)


    try:
        opts, args = getopt.getopt(sys.argv[1:], "" , ["webserver=","dbserver=","rtbserver=","realizeserver=","insaserver=","phase="])
    except getopt.GetoptError, err:
        LOG.error(str(err))
        LOG.error('USAGE: deploy.py --webserver host --dbserver host --rtbserver host [--realizeserver host] --phase phasename')
        sys.exit(1)

    # open logfile and general housekeeping
    for o, a in opts:
        if o in ("--webserver"):
                webserver=a
        elif o in ("--rtbserver"):
                rtbserver=a
        elif o in ("--dbserver"):
                dbserver=a
        elif o in ("--insaserver"):
            try:
                realizeserver,__,__ = socket.gethostbyaddr(a)
            except:
                realizeserver=""
        elif o in ("--realizeserver"):
            try:
                realizeserver,__,__ = socket.gethostbyaddr(a)
            except:
                realizeserver=""
        elif o in ("--phase"):
            phase=a
        else:
            assert False, "unhandled option"

    # check for mandatory options

    if "unset" in (phase, webserver, rtbserver, dbserver):
        LOG.critical('USAGE: deploy.py --webserver host --dbserver host --rtbserver host [--realizeserver host] --phase phasename')
        sys.exit(1)

    hostname =  socket.gethostname()

    if hostname not in (webserver,dbserver,rtbserver):
        LOG.critical('local hostname %s matches no entries on input hostlist' % (hostname))
        sys.exit(1)

    LOG.info('hostlist: \nweb=%s\ndbserver=%s\nrtbserver=%s\nrealizeserver=%s\n' % (webserver, dbserver, rtbserver, realizeserver))

    if phase == "prep":
        doPrep()
    elif phase == "install":
        doInstall()
    elif phase == "upgrade":
        doInstall()
    elif phase == "post":
        doPost()
    elif phase == "license":
        dmvlicense()
    elif phase == "all":
        doPrep()
        doInstall()
        doPost()
    else:
        LOG.error('Unknown phase %s' % (phase))
        sys.exit(1)

    sys.exit(0)

def doInstall():

    # discover Invigorate package
    pfiles = os.listdir('/interact/')

    invpackage = 'notfound'
    for item in pfiles:
        if re.match("Invigorate-.*\.ii",item):
            invpackage=item
            LOG.info('found package %s' % (invpackage))

    if (invpackage == 'notfound'):
        LOG.critical('No Invigorate package found')
        sys.exit(1)
    else:
        LOG.info('Processing Invigorate package: %s ' % invpackage)
        # this will clean up any previous exploded packages
        doCmd("mkdir -p /interact/packages/")
        doCmd("mv /interact/%s /interact/packages/" % invpackage)

    # define an empty list for startup commands

    startlist = []
    if hostname==rtbserver:
        doCmd("chmod 777 /interact/packages/Invigorate-*.ii")
        doCmd("/interact/packages/Invigorate-*.ii --install --force --nolicense --nostart --noinput rtbserver")
        doCmd("chown interact:operator /interact/program/iiMoap")
        LOG.info('Licensing rtbserver with DMV')
        dmvlicense()
        # Installation successful.
        LOG.info('RTB Installation complete')
        # Add rtb processes to list to start
        if os.path.isfile("/etc/init.d/dmv"):
            startlist.append("/etc/init.d/dmv start")
        elif os.path.isfile("/usr/lib/systemd/system/dmv.service"):
            startlist.append("/usr/bin/systemctl start dmv")                    

        if os.path.isfile("/etc/init.d/collector"):
            startlist.append("/etc/init.d/collector start")
        elif os.path.isfile("/usr/lib/systemd/system/collector.service"):
            startlist.append("/usr/bin/systemctl start collector")

        if os.path.isfile("/usr/lib/systemd/system/moap.service"):
            startlist.append("/usr/bin/systemctl start moap")
        elif os.path.isfile("/etc/init.d/vipStart"):
            startlist.append("/etc/init.d/vipStart start")

        # start iiTes if present
        if os.path.isfile("/etc/init.d/tes"):
            doAnyway("/etc/init.d/tes start")
        elif os.path.isfile("/usr/lib/systemd/system/tes.service"):
            doAnyway("/usr/bin/systemctl start tes")
            
        startlist.append("/etc/init.d/ladder start")

    if hostname==dbserver:
        doCmd("chmod 777 /interact/packages/Invigorate-*.ii")
        doCmd("/interact/packages/Invigorate-*.ii --install --force --nolicense --nostart --noinput dbserver")
        # perform the schema installs
        # this is a mess
        doCmd("rm -rf /home/oracle/schema")
        doCmd("cp -r /interact/schema /home/oracle/schema")
        doCmd("chmod -R 777 /home/oracle/schema")
        doCmd("chown -R oracle.dba /home/oracle/schema")
        # explode sample data first to ensure that it gets loaded instead of base
        doCmd("su - oracle -c 'cd /home/oracle/schema/smp*;tar xvzf sample_data.tgz'")
        # install smp schema
        doCmd("su - oracle -c 'cd /home/oracle/schema/smp*;./install_release.sh noprompt'")
        # install dras schema
        doCmd("su - oracle -c 'cd /home/oracle/schema/dras*;./install_release.sh noprompt'")
        # install redeem schema
        doCmd("su - oracle -c 'cd /home/oracle/schema/redeem*; sqlplus sys/super as sysdba @ voucher_clear' ")
        doCmd("su - oracle -c 'cd /home/oracle/schema/redeem*; sqlplus sys/super as sysdba @ voucher_user_create' ")
        doCmd("su - oracle -c 'cd /home/oracle/schema/redeem*; sqlplus \'voucher/deckparty\' @ schema' ")
        doCmd("su - oracle -c 'cd /home/oracle/schema/redeem*; sqlplus \'voucher/deckparty\' @ defaults' ")
        # setup acls for 11g.  This will blow up on a 10g platform
        doAnyway("su - oracle -c 'cd /home/oracle/schema/smp*; sqlplus sys/super as sysdba @ aclsetup' ")
        # and a final revalidation of all objects
        doCmd("su - oracle -c 'cd /home/oracle/schema/smp*; sqlplus sys/super as sysdba @ getinvalids' ")
        LOG.info("Licensing DB with DMV")
        dmvlicense()
        # Installation successful.
        LOG.info("DB Installation complete.")
        # Add db processes to list to start
        
        if os.path.isfile("/etc/init.d/dmv"):
            startlist.append("/etc/init.d/dmv start")
        elif os.path.isfile("/usr/lib/systemd/system/dmv.service"):
            startlist.append("/usr/bin/systemctl start dmv")
            
        if os.path.isfile("/etc/init.d/collector"):
            startlist.append("/etc/init.d/collector start")
        elif os.path.isfile("/usr/lib/systemd/system/collector.service"):
            startlist.append("/usr/bin/systemctl start collector")
        startlist.append("/etc/init.d/drasctl start")

    if hostname==webserver:
        os.environ['d8fe298af9a844a88fb96a9e7dc9e9f0'] = "%s %s %s" % (webserver, dbserver, rtbserver)
        doCmd("chmod 777 /interact/packages/Invigorate-*.ii")
        doCmd("/interact/packages/Invigorate-*.ii --install --force --nolicense --nostart --noinput webserver")

        LOG.info("Licensing webserver with DMV")
        dmvlicense()

        # Installation successful. Start appropriate processes
        LOG.info("WEB Installation complete.")
        # Add web processes to list to start
        if os.path.isfile("/etc/init.d/dmv"):
            startlist.append("/etc/init.d/dmv start")
        elif os.path.isfile("/usr/lib/systemd/system/dmv.service"):
            startlist.append("/usr/bin/systemctl start dmv")
            
        if os.path.isfile("/etc/init.d/collector"):
            startlist.append("/etc/init.d/collector start")
        elif os.path.isfile("/usr/lib/systemd/system/collector.service"):
            startlist.append("/usr/bin/systemctl start collector")

    # execute listed startup commands
    started = [ ]
    for command in startlist:
        if command not in started:
            doCmd(command)
            started.append(command)


def doPost():
    #postlist
    if hostname==webserver:
        doCmd("echo '%s\:8080=Other Webserver' > /interact/tomcat/shared/classes/statServers.properties" % socket.gethostname())
        doCmd("/etc/init.d/tomcat start")
        doCmd("su - interact -c '/interact/bin/serverMonitor.sh --startall'")
    if hostname==dbserver:
        doCmd("su - oracle -c 'xtt -o reset'")
        doCmd("su - oracle -c 'xtt -o imp -n -s 1 -D /home/oracle/schema/smp*'")
        doCmd("su - oracle -c 'xtt -o imp -n -s 2 -D /home/oracle/schema/smp*'")
        doCmd("su - oracle -c 'xtt -o imp -n -s 3 -D /home/oracle/schema/smp*'")
        doCmd("su - oracle -c 'xtt -o imp -n -s 4 -D /home/oracle/schema/smp*'")
        doCmd("su - oracle -c 'xtt -o imp -n -s 5 -D /home/oracle/schema/smp*'")
        doCmd("su - oracle -c 'xtt -o imp -n -s 6 -D /home/oracle/schema/smp*'")

        # hack in some ugly for the LOAD_BALANCE crap
        sqlTemp=open("/tmp/sqltmp.sql","w+")
        sqlTemp.write("set echo on;\nupdate jsecure.load_balance set machine='%s';\ncommit work;\nexit;\n" % (webserver))
        sqlTemp.close()
        doAnyway("su - oracle -c 'sqlplus jsecure/deckparty @ /tmp/sqltmp' ")
        sqlTemp=open("/tmp/sqltmp.sql","w+")
        sqlTemp.write("set echo on;\nupdate smp.refresh set remote_host='%s';\ncommit work;\nexit;\n" % (webserver))
        sqlTemp.close()
        doAnyway("su - oracle -c 'sqlplus smp/deckparty @ /tmp/sqltmp' ")
    if hostname==rtbserver:
        doCmd("su - interact -c '/interact/invigorate-db/TimesTen/tt1122/bin/ttisql -f /interact/invigorate-db/schema/seed.sql INVIGORATE'")
if __name__ == "__main__": main()
