<?php
/* Start Portal specific configuration */

/* Show YES/NO boolean as radio buttons */
$ADD_RADIO_BUTTONS = true;
/* Portal Request Email Recipient for template based email forms */
$ADMIN_EMAIL = '';
/* Asset Fulfillment Request delivery volume root */
$AFR_FULFILLMENT_VOLUME = '';
/* Enable MView annotation tools */
$ANNOTATIONS_DISPLAY = true;
/* Amount of time since a user views/edits annotations they are considered "active."  In seconds. */
$ANNOTATION_ACTIVITY_TIMEOUT = '300';
/* Show Approval Manager button */
$APPROVALMGR_DISPLAY = false;
/* Show Basket button */
$BASKET_DISPLAY = true;
/* Number of Breadcrumb segments to display as path. 0 for all segments */
$BREADCRUMB_LENGTH = '5';
/* Show Browse button */
$BROWSE_FOLDER_DISPLAY = true;
/* Clear old search queries on search options load */
$CLEAR_SEARCHOPTIONS = false;
/* Enable/Disable DALiM DiALOGUE server access button {DB} */
$DALIM_DIALOG = false;
/* IP address/Domain name of DiALOGUE server */
$DIALOG_SERVER = '';
/* TCP port of DiALOGUE server */
$DIALOG_SERVER_PORT = '';
/* Set the number of columns for displaying Directory list. Zero for automatic. */
$DIRECTORY_COLUMNS = '1';
/* Number of directories to show per page. */
$DIRECTORY_PERPAGE = '96';
/* Sort the Directories into columns = true OR rows = false */
$DIRECTORY_SORTBY_COLUMNS = false;
/* Show Distributor button */
$DISTRIBUTOR_DISPLAY = false;
/* Show FPO download button */
$DOWNLOAD_FPO_DISPLAY = true;
/* Show Hires download button */
$DOWNLOAD_HIRES_DISPLAY = true;
/* Enables CSRF prevention. */
$ENABLE_CSRF_CHECKS = false;
/* Enable portalDI debugging. Output is sent to Venture log. */
$ENABLE_PDI_DEBUGGING = false;
/* Enable users to save search options. */
$ENABLE_SAVEDSEARCHES = true;
/* Show File Manager button */
$FILE_MANAGER_DISPLAY = true;
/* Number of files to show per page. */
$FILE_PERPAGE = '24';
/* Show Gallery view button */
$GALLERY_VIEW_DISPLAY = false;
/* Force Portal to generate all available TAGS (performance hit for enabling this option). */
$GENERATE_ALL_PORTAL_TAGS = false;
/* Hide buttons when disabled or inactive. */
$HIDE_INACTIVE_BUTTON = true;
/* Show Icon view button */
$ICON_VIEW_DISPLAY = true;
/* Show ImageInfo button */
$IMAGEINFO_DISPLAY = false;
/* Set KW_NAME to use localized values from language.js file on WebNative server */
$KEYWORD_LOCALIZEDNAMES = true;
/* Keyword name and value separator */
$KEYWORD_SEPARATOR = '';
/* Set a range of years for portal to use in the drop down menu of date type keyword fields */
$KEYWORD_YEAR_RANGE = '20';
/* Set a start year for the range setting above. Empty value is current year minus (Keyword_Year_Range/2) */
$KEYWORD_YEAR_START = false;
/* Set the Max dimension in pixels of the LARGE previews longest side. */
$LARGE_PREVIEW_MAX = '1100';
/* Large preview padding color */
$LARGE_PREVIEW_PADDING_COLOR = '#646464';
/* Show List view button */
$LIST_VIEW_DISPLAY = false;
/* Message to display when user can not be authenticated to WebNative server */
$LOGIN_FAIL_MSG = 'Incorrect User Name and/or Password';
/* Message to display when an account has been Administratively disabled on WebNative server */
$LOGIN_REJECT_MSG = 'Login account has been disabled';
/* Show Logout button */
$LOGOUT_DISPLAY = true;
/* Show Long view button */
$LONG_VIEW_DISPLAY = true;
/* Maximum number of connections per username. 0 for unlimited */
$MAX_CONCURRENT_SESSIONS = '10';
/* Message to display when MAX_SESSION_INACTIVITY is hit */
$MAX_INACTIVITY_MSG = 'User session has expired';
/* Maximum time in seconds a user can be idle. 0 for unlimited  */
$MAX_SESSION_INACTIVITY = '14400';
/* Message to display when MAX_CONCURRENT_SESSIONS is hit */
$MAX_USER_MSG = 'The user name submitted has an active session. If you feel this message is an error please contact your system administrator';
/* Show Mview button */
$MVIEW_DISPLAY = true;
/* Use the Portal Navigator */
$NAVIGATOR = true;
/* Show files in the Portal Navigator */
$NAVIGATOR_SHOWIMGS = false;
/* Make keyword values available in the Portal Navigator */
$NAVIGATOR_ENABLEKYWDS = false;
/* Include facets in Top Level */
$NAVIGATOR_TOPFACETS = false;
/* Show Open File Annotations button. Requires AssetBrowser */
$OPENFILEANNOTATIONS_DISPLAY = false;
/* Show Open High button. Requires AssetBrowser */
$OPENHIGH_DISPLAY = false;
/* Show Custom Order button */
$ORDER_DISPLAY = true;
/* When true {SP} tag will always be a small preview even in Icon View */
$OVERRIDE_SP = true;
/* Wrap SWF files with flash player object. */
$PLAY_SWF_FILES = false;
/* Disabling this option may impair functionality and make future functionality unavailable. */
$PORTAL_MAINTAINED = true;
/* Show User Preferences button */
$PREFERENCE_DISPLAY = true;
/* Show Push to DAM button */
$PUSHTODAM_DISPLAY = false;
/* Predefined quick search settings from searchengine. Zero for default SEARCHALL. */
$QUICK_SEARCH = '0';
/* Show Report button. */
$REPORT_DISPLAY = false;
/* Show Reveal High button. Requires AssetBrowser */
$REVEALHIGH_DISPLAY = false;
/* Include archived files unless OTHERWISE specified by user */
$SEARCH_ARCHIVE_DEFAULT = false;
/* Show Search button */
$SEARCH_DISPLAY = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_ANNOTATIONS = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_ARCHIVED = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_COMMENT = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_DATE = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_EVENTS = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILECOLORSPACE = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILECONTENT = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILEDIR = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILEHEIGHT = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILEICCPROFILE = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILENAME = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILERES = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILESIZE = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILETYPE = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILEWIDTH = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILEWIDTHANDHEIGHT = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_FILEWIDTHORHEIGHT = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_SEARCHALL = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_SEARCHALLFTI = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_SEARCHALLKEYWORD = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_VIDEOBITRATE = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_VIDEOCODEC = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_VIDEODURATION = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_VIDEOFORMAT = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_VIDEOHEIGHT = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_VIDEOWIDTH = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_VIDEOWIDTHANDHEIGHT = true;
/* Enable/Disable as searchable field in drop-menu */
$SEARCH_FIELD_VIDEOWIDTHORHEIGHT = true;
/* BLANK first option for popup lists. */
$SEARCH_FIRST_OPTION_BLANK = false;
/* Sendmail server address and port */
$SENDMAIL_SERVER = '';
/* Allow sharing of saved searches between users of the same primary group */
$SHARE_SAVEDSEARCHES = true;
/* Show Short view button */
$SHORT_VIEW_DISPLAY = true;
/* Display read only Keywords with no values */
$SHOW_EMPTY_KEYWORDS = false;
/* Use finder color labels (when assigned) to pad previews */
$SHOW_FINDER_COLORS = true;
/* Display folders in the main area */
$SHOW_FOLDERS = true;
/* When users only have one assigned volume Portal goes directly to BROWSE */
$SKIP_TOPLEVEL = false;
/* Set the Max dimension in pixels of the SMALL previews longest side. */
$SMALL_PREVIEW_MAX = '112';
/* Small preview padding color */
$SMALL_PREVIEW_PADDING_COLOR = '#646464';
/* For SSO, remove the server name or Kerberos realm, from the users name, for WebNative authentication check */
$SSO_NOSERVERNAME = '';
/* For Kerberos SSO, the Kerberos Realm assigned to the OD Password Service */
$SSO_REALM = '';
/* For SSO, the shared secret password between WebNative server and Portal server */
$SSO_SHARED_SECRET = '';
/* Show Stream button */
$STREAM_DISPLAY = true;
/* Show Approve button in Annotations/Approval tab */
$TABAPPROVE_DISPLAY = false;
/* Show Cancel button in Annotations/Approval tab */
$TABCANCEL_DISPLAY = false;
/* Show Close button in Annotations tab */
$TABCLOSE_DISPLAY = false;
/* Show Reject button in Annotations/Approval tab */
$TABREJECT_DISPLAY = false;
/* Maximum number of text runs to display, per page */
$TEXT_RUN_MAX = '250';
/* Show Filter Options */
$TOOL_DISPLAY = true;
/* Tool tips on mouse over in browser */
$TOOL_TIPS = true;
/* Show Toplevel button */
$TOPLEVEL_DISPLAY = true;
/* Truncate from center. Else truncate off end. */
$TRUNCATE_CENTER = false;
/* Truncate long file and directory names. */
$TRUNCATE_FILENAME = true;
/* Number of characters for truncate */
$TRUNCATE_LENGTH = '24';
/* Truncate long volume names. */
$TRUNCATE_VOLUMENAME = true;
/* Show Upload button */
$UPLOAD_DISPLAY = true;
/* Show preview in Compare tool. */
$USE_VERSIONS_IN_COMPARE = false;
/* Show Versions button */
$VERSIONS_DISPLAY = true;
/* Show View Options */
$VIEW_DISPLAY = true;
/* Set the number of columns for displaying Volume list. Zero for automatic. */
$VOLUME_COLUMNS = '1';
/* Sort the Volumes into columns = true OR rows = false */
$VOLUME_SORTBY_COLUMNS = false;
/* IP address/Domain name of WebNative server : Port number */
$WNHOSTNAME  =  '';

/* End Portal specific configuration */

$ICON_AFR = 'wnp_afr_24.png';

$ICON_ALSO_OFFLINE = 'also.offline_24.png';

$ICON_ANNOTATIONS = 'annotations_24.png';

$ICON_ANNOTATIONS_CHK = 'annotations_chk_24.png';

$ICON_APPROVAL = 'approval_24.png';

$ICON_APPROVALTAB = 'approvaltab.png';

$ICON_APPROVAL_APPROVE = 'approve.gif';

$ICON_APPROVAL_CANCEL = 'approval_cancel.gif';

$ICON_APPROVAL_JOBS = 'jobs_24.png';

$ICON_APPROVAL_MANAGER = 'manager_24.png';

$ICON_APPROVAL_NEWJOB = 'newjob_24.png';

$ICON_APPROVAL_OPENDIR = 'approval_opendir.gif';

$ICON_APPROVAL_OPENFILE = 'approval_openfile.gif';

$ICON_APPROVAL_REJECT = 'reject.gif';

$ICON_APPROVAL_VIEW = 'approvalview.gif';

$ICON_ARROW_CLOSED = 'arrow.closed_24.png';

$ICON_ARROW_OPEN = 'arrow.open_24.png';

$ICON_BASKET = 'basket_24.png';

$ICON_BASKETADMIN = 'WNBASKET_24.png';

$ICON_BASKET_CHECKED = 'basket.checked_24.png';

$ICON_BASKET_VIEW = 'basket_24.png';

$ICON_BATCHAPPLY = 'batch.apply_24.png';

$ICON_BATCHORDER = 'wn_order_24.png';

$ICON_BREADCRUMB_DELIMITER = 'arrow.delimiter_24.png';

$ICON_BROWSE = 'folder_24.png';

$ICON_CHANGE_PASSWORD = 'change.password_24.png';

$ICON_CLOSE = 'close.gif';

$ICON_DIALOG_BUTTON = 'dialog_24.png';

$ICON_DOWNLOADBASKET = 'wn_high_24.png';

$ICON_DOWNLOADBASKETFPO = 'wn_low_24.png';

$ICON_DOWNLOADGIFS = 'wnp_downgif_24.png';

$ICON_DOWNLOADJPGS = 'wnp_downjpg_24.png';

$ICON_DOWNLOADTOSSPS = 'wnp_downtossps_24.png';

$ICON_FILEMNGR = 'filemngr.move_24.png';

$ICON_FILEMNGR_CANCEL = 'filemngr.cancel_24.png';

$ICON_FILEMNGR_COPY = 'filemngr.copy_24.png';

$ICON_FILEMNGR_MKDIR = 'folder.new_24.png';

$ICON_FILEMNGR_MOVE = 'filemngr.move_24.png';

$ICON_FILEMNGR_RENAME = 'filemngr.move_24.png';

$ICON_FILEMNGR_TRASH = 'filemngr.trash_24.png';

$ICON_FOLDER = 'folder_24.png';

$ICON_FOLDER_BIG = 'folder_48.png';

$ICON_FOLDER_CLOSED = 'folder_24.png';

$ICON_FOLDER_NEW = 'folder.new_24.png';

$ICON_FOLDER_OPEN = 'folder.open_24.png';

$ICON_FOLDER_OPEN_SELECTED = 'folder.open.selected_24.png';

$ICON_GALLERY_VIEW = 'gallery.view_24.png';

$ICON_GALLERY_VIEW_ON = 'gallery.view.on_24.png';

$ICON_HIGH = 'high_24.png';

$ICON_HISTORY = 'history_24.png';

$ICON_HOME = 'home_24.png';

$ICON_ICON_VIEW = 'icon.view_24.png';

$ICON_ICON_VIEW_ON = 'icon.view.on_24.png';

$ICON_INFO = 'info_24.png';

$ICON_LEFTARROW = 'leftarrow_24.png';

$ICON_LINKS = 'links_24.png';

$ICON_LOGOUT = 'logout_24.png';

$ICON_LONG_VIEW = 'long.view_24.png';

$ICON_LONG_VIEW_ON = 'long.view.on_24.png';

$ICON_LOW = 'low_24.png';

$ICON_MULTISEARCH = 'multisearch_24.png';

$ICON_NEARLINE = 'nearline_24.png';

$ICON_OFFLINE = 'offline_24.png';

$ICON_ORDER = 'order_24.png';

$ICON_PDF = 'pdfbutton_24.png';

$ICON_PRINTER = 'printer_24.png';

$ICON_PROMOTE = 'promote_24.png';

$ICON_QMARK = 'qmark_24.png';

$ICON_QUARK_FULL = 'quarkiconfull_24.png';

$ICON_QUARK_PART = 'quarkiconpart_24.png';

$ICON_REPORT = 'report_24.png';

$ICON_RIGHTARROW = 'rightarrow_24.png';

$ICON_SEARCH = 'search_24.png';

$ICON_SHIP = 'ship_24.png';

$ICON_SHORT_VIEW = 'short.view_24.png';

$ICON_SHORT_VIEW_ON = 'short.view.on_24.png';

$ICON_STREAM = 'stream_24.png';

$ICON_TRASH = 'trash_24.png';

$ICON_UPLOAD = 'upload_24.png';

$ICON_VERSIONS = 'versions_24.png';

$ICON_VIDEOGENPLUGIN = 'movie_24.png';

$ICON_WRENCH = 'wrench_24.png';

/* Theme selected for this site */
$THEME = 'Exhibit';

?>

