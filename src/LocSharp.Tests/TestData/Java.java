package com.interact.diameter.request;

import java.util.Date;
import java.util.HashSet;
import java.util.Set;

/**
 * Credit request object, used for inputs to Credit.
 */
public class CreditRequest extends BaseRequest
{
        private static final Logger log = Logger.getLogger(InvigorateDiameterBase.class);
    private static final String configFile = "diameter_config.xml";
    private static final String SCTP_CONNECTION_VALUE = "org.jdiameter.client.impl.transport.sctp.SCTPClientConnection";
    private static Stack stack;

    private static int vendorId = 0;
    private static ApplicationId applicationID;
    private static XMLConfiguration config;
    protected static boolean stubMode = false;
    protected static int requestTimeout = 0;
    protected static String realmName = "invigorateRTB";
    private static String defaultPrimary;
    private static String defaultSecondary;

    protected static final String INVX_SERVICE_CONTEXT_ID = "16.0.0@newnet.com";
    protected static final int INVX_EVENT_REQUEST = 4;
    protected static final int INVX_REQUEST_NUMBER = 0;
    protected static final int INVX_SUBSCRIPTION_ID_TYPE = 0;


    public static final int RESULT_DIAMETER_SUCCESS = 2001;
    public static final int RESULT_DIAMETER_TIMEOUT = 3004;
    protected static final int RESULT_INVALID_AVP_VALUE = 5004;
    protected static final int RESULT_DIAMETER_MISSING_AVP = 5005;
    protected static final int RESULT_DIAMETER_USER_UNKNOWN = 5005;

    public static final String ACTIVE_STATE = "Active";
    public static final String SUSPENDED_STATE = "Suspended";

    private static boolean isRangeRecEnabled = false;

    private static List<String> peerList;
    Double moneyAmount;
    String requestCode = null;
    Set<PartitionRequest> partitionRequests = new HashSet<PartitionRequest>();

    @Override
    public void setTransactionId(String transactionId)
    {
        this.transactionId = transactionId;
    }

    @Override
    public void setSubscriber(String subscriber)
    {
        this.subscriber = subscriber;
    }

    static
    {
        try
        {
            String sVendorId = PropertiesLoader.get("vendorId");
            if(sVendorId == null || sVendorId.equals(""))
            {
                log.warn("Unable to retrieve vendorId from configuration. Using default[" + vendorId + "]");
            }
            else
            {
                vendorId = Integer.parseInt(sVendorId);
            }

            String sAppId = PropertiesLoader.get("authAppId");
            if(sAppId == null || sAppId.equals(""))
            {
                log.warn("Unable to retrieve authAppId from configuration. Using default[4]");
                sAppId = "4";
            }

            String sRealmName = PropertiesLoader.get("realm");
            if(sRealmName == null || sRealmName.equals(""))
            {
                log.warn("Unable to retrieve realm from configuration. Using default[" + realmName + "]");
            }
            else
            {
                realmName = sRealmName;
            }

            String sRequestTimeout = PropertiesLoader.get("request.timeout");
            if(sRequestTimeout != null && !sRequestTimeout.equals(""))
            {
                requestTimeout = Integer.parseInt(sRequestTimeout);
            }
            else
            {
                log.error("Invalid request.timeout configuration parameter.");
                requestTimeout = Integer.MAX_VALUE;
            }

            String sdefaultPrimary = PropertiesLoader.get("default.primary");
            if(sdefaultPrimary != null && !sdefaultPrimary.equals(""))
            {
                defaultPrimary = sdefaultPrimary;
            }
            else
            {
                log.warn("No Default Primary host configured.");
                defaultPrimary = null;
            }

            String sdefaultSecondary = PropertiesLoader.get("default.secondary");
            if(sdefaultSecondary != null && !sdefaultSecondary.equals(""))
            {
                defaultSecondary = sdefaultSecondary;
            }
            else
            {
                log.warn("No Default Secondary host configured.");
                defaultSecondary = null;
            }

            applicationID = ApplicationId.createByAuthAppId(vendorId, Integer.parseInt(sAppId));

            //Load diameter xml configuration
            InputStream configStream = InvigorateDiameterBase.class.getClassLoader().getResourceAsStream(configFile);
            config = new XMLConfiguration(configStream);

            Configuration[] thisConfig = config.getChildren(33);
            if(thisConfig.length > 0)
            {
                String clientConnection = thisConfig[0].getStringValue(7, null);
                log.debug("Client connection: " + clientConnection);
                if(clientConnection != null && clientConnection.equalsIgnoreCase(SCTP_CONNECTION_VALUE))
                {
                    //This client is attempting to connect as SCTP. Set the environment variable for
                    // "sctp.persist.dir", if configured
                    if(PropertiesLoader.containsKey("sctp.persist.dir"))
                    {
                        String sctp_persist_dir = PropertiesLoader.get("sctp.persist.dir");
                        if(sctp_persist_dir != null && !sctp_persist_dir.equals(""))
                        {
                            System.setProperty("sctp.persist.dir", sctp_persist_dir);
                            log.info("Set sctp.persist.dir[" + System.getProperty("sctp.persist.dir") + "]");
                        }
                    }
                }
            }

            Configuration[] thisPeerConfig = config.getChildren(35);
            if(thisPeerConfig.length > 0)
            {
                peerList = new ArrayList<String>(thisPeerConfig.length);
                for(int i = 0; i < thisPeerConfig.length; i++)
                {
                    String thisPeerString = thisPeerConfig[i].getStringValue(17, null);
                    log.debug("Peer [" + i + "] : " + thisPeerString);
                    String[] splitPeer = thisPeerString.split(":");
                    if(splitPeer.length != 3)
                    {
                        log.error("Invalid PEER configuration: " + thisPeerString);
                    }
                    else
                    {
                        String theHost = splitPeer[1];
                        theHost = theHost.replaceAll("//", "");
                        log.debug("Final Peer [" + theHost + "]");
                        theHost = theHost.toLowerCase();
                        peerList.add(theHost);
                    }
                }
            }

            String stubString = System.getProperty("diameterStub");
            if(stubString != null && !stubString.equals(""))
            {
                if(stubString.equalsIgnoreCase("true"))
                {
                    log.warn("CONFIGURED FOR STUB MODE");
                    stubMode = true;
                }
            }
        }
        catch(FileNotFoundException fnfe)
        {
            log.fatal("Unable to load diameter configuration file[" + configFile + "]");
        }
        catch(Exception e)
        {
            log.fatal("Exception parsing diameter configuration file[" + configFile + "] : " + e.getMessage());
        }

    }

    /**
     * Set the money amount of the credit.
     *
     * @param moneyAmount Value of the money credit.
     */
    public void setMoneyAmount(Double moneyAmount)
    {
        this.moneyAmount = moneyAmount;
    }

    /**
     * Get the money amount of the request.
     *
     * @return Money amount of the credit.
     */
    public Double getMoneyAmount()
    {
        return moneyAmount;
    }

    /**
     * Get the request code value.
     *
     * @return Request code.
     */
    public String getRequestCode()
    {
        return requestCode;
    }

    /**
     * Set the request code.
     *
     * @param requestCode Request code.
     */
    public void setRequestCode(String requestCode)
    {
        this.requestCode = requestCode;
    }

    /**
     * Add a partition credit request.
     *
     * @param name             Partition name.
     * @param balanceChange    Amount to credit the balance.
     * @param bonusChange      Amount to credit the bonus balance.
     * @param balanceExtension Number of days to extend the balance.
     * @param bonusExtension   Number of days to extend the bonus balance.
     */
    public void addPartition(String name, int balanceChange, int bonusChange, int balanceExtension, int bonusExtension)
    {
        PartitionRequest thisRequest =
                new PartitionRequest(name, balanceChange, bonusChange, balanceExtension, bonusExtension);
        partitionRequests.add(thisRequest);
    }

    /**
     * Set the partition request object set.
     *
     * @param partitionRequests Set of partition request objects.
     */
    public void setPartitionRequests(Set<PartitionRequest> partitionRequests)
    {
        this.partitionRequests = partitionRequests;
    }

    /**
     * Get the partition request objects.
     *
     * @return Set of PartitionRequest objects.
     */
    public Set<PartitionRequest> getPartitions()
    {
        return partitionRequests;
    }

    public class PartitionRequest
    {
        private String partitionName;
        private int balanceChange;
        private int bonusChange;
        private int balanceExtension;
        private int bonusExtension;

        /**
         * Create a PartitionRequest object.
         *
         * @param partitionName    Partition id/name.
         * @param balanceChange    Amount to change the balance.
         * @param bonusChange      Amount to change the bonus balance.
         * @param balanceExtension Number of days to extend the balance.
         * @param bonusExtension   Number of days to extend the bonus balance.
         */
        PartitionRequest(String partitionName, int balanceChange, int bonusChange, int balanceExtension,
                         int bonusExtension)
        {
            this.partitionName = partitionName;
            this.balanceChange = balanceChange;
            this.bonusChange = bonusChange;
            this.balanceExtension = balanceExtension;
            this.bonusExtension = bonusExtension;
        }

        /**
         * Default constructor.
         */
        public PartitionRequest()
        {
            //Default constructor
        }

        /**
         * Get the partition name.
         *
         * @return Partition name.
         */
        public String getPartitionName()
        {
            return partitionName;
        }

        /**
         * Get the balance change value.
         *
         * @return Balance change value.
         */
        public int getBalanceChange()
        {
            return balanceChange;
        }

        /**
         * Get the bonus balance change value.
         *
         * @return Bonus change value.
         */
        public int getBonusChange()
        {
            return bonusChange;
        }

        /**
         * Get the balance extension value.
         *
         * @return Balance extension value.
         */
        public int getBalanceExtension()
        {
            return balanceExtension;
        }

        /**
         * Get the bonus extension value.
         *
         * @return Bonus extension value.
         */
        public int getBonusExtension()
        {
            return bonusExtension;
        }

        /**
         * Set the partition name.
         *
         * @param partitionName Partition name.
         */
        public void setPartitionName(String partitionName)
        {
            this.partitionName = partitionName;
        }

        /**
         * Set the balance change value.
         *
         * @param balanceChange Balance change.
         */
        public void setBalanceChange(int balanceChange)
        {
            this.balanceChange = balanceChange;
        }

        /**
         * Set the bonus balance change value.
         *
         * @param bonusChange bonus change value.
         */
        public void setBonusChange(int bonusChange)
        {
            this.bonusChange = bonusChange;
        }

        /**
         * Set the balance extension value.
         *
         * @param balanceExtension Balance extension value.
         */
        public void setBalanceExtension(int balanceExtension)
        {
            this.balanceExtension = balanceExtension;
        }

        /**
         * Set the bonus extension value.
         *
         * @param bonusExtension Bonus balance extension value.
         */
        public void setBonusExtension(int bonusExtension)
        {
            this.bonusExtension = bonusExtension;
        }
    }
}



