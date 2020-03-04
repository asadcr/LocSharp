param($RepoName)

if([string]::IsNullOrEmpty($RepoName)) {
    Write-Host "Please provide RepoName."

    return;
}

New-Item -Path "codeanalysis_outputs" -ItemType Directory -Force | Out-Null

# Extracting Code Lines Information...
Write-Host "Extracting Code Lines Information..."

cloc --csv --by-file --strip-str-comments --skip-uniqueness ./ > codeanalysis_outputs\cloc.txt 
github-linguist ./ --breakdown > codeanalysis_outputs\linguist.txt

# Extracting Revision History...
Write-Host "Extracting Revision History..."

git log --date=iso --numstat --diff-filter=ACDMRTUXB --full-history -- . > codeanalysis_outputs\hotspots.txt
git log --summary --name-status --diff-filter=R --full-history -- . > codeanalysis_outputs\hotspots_renames.txt

# Extracting Hardcoded Items...
Write-Host "Extracting Hardcoded Items..."

<#
These following commands extract the hardcoded items from the codebase itself
Take a look at the regexes
#>

git grep -I --ignore-case --line-number --column --perl-regexp "(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])?\d{1,2}" > codeanalysis_outputs\hardcoded_ips.txt
git grep -I --ignore-case --line-number --column --perl-regexp "[\'\""]((?<ab>([a-zA-Z]\:[\/\\]+)|(\/|\\)?(usr|bin|mnt|var|[rb]oot|etc|home)(\/|\\))|(?<re>\.{1,2}[\/\\]{1,2}))+[\w-\/\\]+.*?[\'\""]" > codeanalysis_outputs\hardcoded_files.txt
git grep -I --ignore-case --line-number --column --perl-regexp "(?<g1>(https?|ftp|svn):\/\/[\w\-_]+(\.[\w\-_]+)+)|([\'\""](?<g2>([\w\-_]+\.)+(net|com|org|io|tk|de|ru|uk|br|ir|pl|it|jp|fr|nl|ca|au|es|ch|edu|gov|se|us|no))[\'\""\/])" > codeanalysis_outputs\hardcoded_domains.txt
git grep -I --ignore-case --line-number --column --perl-regexp "(api|gitlab|github|slack|google|aws|token)(_?)(id|key|token|secret)?([\""\']?\s*[!=:]{1,3}\s*[\""\'])(?<value>[^\""\'\>\<\~\^].*)([\""\'][;,]?)" > codeanalysis_outputs\hardcoded_apikeys.txt
git grep -I --ignore-case --line-number --column --perl-regexp "(password|passwd|pwd|server|connection|connectionstring|database|db|credentials)([\""\']?\s*[!=:]{1,3}\s*[\""\'])(?<value>[^\""\'\>\<\~\^].*)([\""\'][;,]?)" > codeanalysis_outputs\hardcoded_passwords.txt
git grep -I --ignore-case --line-number --column --perl-regexp "(usr|user|username|uid)([\""\']?\s*[!=:]{1,3}\s*[\""\'])(?<value>[^\""\'\>\<\~\^].*)([\""\'][;,]?)" > codeanalysis_outputs\hardcoded_userids.txt
git grep -I --ignore-case --line-number --column --perl-regexp "(-{5}BEGIN\s*.{1,15}\s*PRIVATE KEY\s*-{5})(?<value>.*)" > codeanalysis_outputs\hardcoded_privatekeys.txt

# Extracting License Compliance Information...
Write-Host "Extracting License Compliance Information..."

# Finds the object
function Resolve-Object ($file) {
    # File is In Path
	if ($IsLinux) {
		return which $file
	} else {
        $pathSeparator = [IO.Path]::PathSeparator
        $path =  (".${pathSeparator}" + $env:PATH).Split($pathSeparator) | Where-Object {return (-not ([string]::IsNullOrEmpty($_))) -and (Test-Path (Join-Path $_  $file)) } | Select-Object -First 1
        
        return Join-Path $path  $file
	}
}

$jarPath = Resolve-Object -file wss-agent.jar
$configPath = Resolve-Object -file ws-agent-cfg.config

$argsToPass = ("-jar $($jarPath)"," -d ./", "-c $($configPath)", "-project ""$($RepoName)""", "-apiKey apiKey", "-offline true")

$ScanProcess = Start-Process "java" -NoNewWindow -PassThru -ArgumentList $argsToPass -Wait
$ScanProcessExitCode = $ScanProcess.ExitCode;

Write-Host $ScanProcessExitCode

if($ScanProcess.ExitCode -eq 0) {
    Move-Item -Path "./whitesource/update-request.txt" -Destination "./codeanalysis_outputs/whitesource_scan.txt" -Force
    Remove-Item -LiteralPath "./whitesource" -Force -Recurse
}
