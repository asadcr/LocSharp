namespace CAFlow.Core.Services
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Linq;
    using System.Net.Http;
    using System.Threading.Tasks;
    using Common.Extensions;
    using Common.Models;
    using Common.Utils;
    using Data.DbContext;
    using Data.Models.CAFlow;
    using Interfaces;
    using Microsoft.EntityFrameworkCore;
    using Microsoft.Extensions.Logging;
    using Microsoft.Extensions.Options;
    using Models.CodeGraph;

    public class CodeGraphService : ICodeGraphService
    {
        // Cypher (Neo4j) query to retrieve classes and their coupling values.
        // NOTE: we count COUPLE relationships, which include only couplings to 1st party classes.
        // We can also use n.CountClassCoupled field directly to include coupling to 3rd party classes
        // (including system, framework, and libraries).
        private const string CouplingsQuery =
            "MATCH (n:TypeDeclaration) " +
            "WHERE n.file IS NOT NULL " +
            "  AND n.longname IS NOT NULL " +
            "  AND n.CountClassCoupled IS NOT NULL " +
            "  AND n.is_external IS NULL " + // Exclude system types
            "RETURN n.file, n.longname, size((n)-[:COUPLE]->(:TypeDeclaration)) as CountClassCoupled";

        private const string ReqActivateSandboxUrl = "/codegraph/builds/{0}/sandbox/activate/";
        private const string ReqDescribeSandboxUrl = "/codegraph/builds/{0}/sandbox/";

        private static readonly TimeSpan SandboxActivationRetryInterval = TimeSpan.FromSeconds(5);

        private readonly CAFlowDbContext _caFlowDb;
        private readonly IHttpClientFactory _httpClientFactory;
        private readonly ILogger<CodeGraphService> _logger;

        private readonly string _token;
        private readonly string _apiUrl;
        private readonly string _sandboxUser;
        private readonly string _sandboxPassword;

        public CodeGraphService(
            CAFlowDbContext caFlowDb,
            IHttpClientFactory httpClientFactory,
            IOptions<CodeGraphOptions> options,
            ILogger<CodeGraphService> logger)
        {
            _caFlowDb = Arg.NotNull(caFlowDb, nameof(caFlowDb));
            _httpClientFactory = Arg.NotNull(httpClientFactory, nameof(httpClientFactory));
            _token = Arg.NotNullOrWhitespace(options?.Value?.Token, nameof(CodeGraphOptions.Token));
            _apiUrl = Arg.NotNullOrWhitespace(options?.Value?.ApiUrl, nameof(CodeGraphOptions.ApiUrl));
            _sandboxUser = Arg.NotNullOrWhitespace(options?.Value?.SandboxUser, nameof(CodeGraphOptions.SandboxUser));
            _sandboxPassword = Arg.NotNullOrWhitespace(options?.Value?.SandboxPassword, nameof(CodeGraphOptions.SandboxPassword));
            _logger = Arg.NotNull(logger, nameof(logger));
        }

        /// <summary>
        /// Gets or sets a value indicating whether CodeGraph feature is enabled.
        /// TEMPORARY PROPERTY. Remove when CG issues are sorted out.
        /// </summary>
        public static bool FeatureEnabled { get; set; }

        public async Task<IOperationResult> PopulateClassesAsync(Guid branchId, string codeGraphRequestId)
        {
            var sandbox = await GetActivatedSandboxAsync(codeGraphRequestId);
            var typeDeclarations = await QueryAsync(sandbox.BrowserUrl, CouplingsQuery);
            if (typeDeclarations?.Data == null)
            {
                return OperationResult.FailMessage("Failed to load CodeGraph data: empty result");
            }

            var oldClasses = await _caFlowDb.FileClasses
                .Where(c => c.File.BranchId == branchId)
                .Select(c => c.Class)
                .ToListAsync();

            _caFlowDb.RemoveRange(oldClasses);
            await _caFlowDb.SaveChangesAsync();

            var newClasses = new Dictionary<string, Class>();

            var files = await _caFlowDb.Files
                .Where(f => f.BranchId == branchId)
                .ToDictionaryAsync(f => f.FullName, f => f.Id);

            int matchCount = 0;

            foreach (var typeDeclaration in typeDeclarations.Data)
            {
                var file = (string) typeDeclaration[0];
                var longName = (string) typeDeclaration[1];

                // For some reason this field can be returned as number or as string from CodeGraph.
                var countClassCoupled = Convert.ToInt32(typeDeclaration[2], CultureInfo.InvariantCulture);

                var fileId = files.GetValueOrDefault(file);
                if (fileId == default)
                {
                    _logger.LogDebug("CodeGraph file not found: " + file);
                    continue;
                }

                var cls = newClasses.GetValueOrDefault(longName);
                if (cls == null)
                {
                    cls = new Class
                    {
                        Name = longName,
                        FileClasses = new List<FileClass>()
                    };
                    _caFlowDb.Classes.Add(cls);
                    newClasses.Add(longName, cls);
                }

                cls.FileClasses.Add(new FileClass { FileId = fileId });
                cls.CoupledClassCount = countClassCoupled;

                matchCount++;
            }

            await _caFlowDb.SaveChangesAsync();

            return OperationResult.SuccessMessage($"Loaded {typeDeclarations.Data.Count} type declarations " +
                                                  $"from '{codeGraphRequestId}' sandbox, matched {matchCount}.");
        }

        private async Task<Sandbox> GetActivatedSandboxAsync(string codeGraphRequestId)
        {
            var http = _httpClientFactory.CreateClient();

            // Activate.
            _logger.LogDebug($"Activating CodeGraph sandbox {codeGraphRequestId}");
            var reqActivateSandbox = string.Format(CultureInfo.InvariantCulture, ReqActivateSandboxUrl, codeGraphRequestId);
            var activateUrl = $"{_apiUrl}{reqActivateSandbox}?token={_token}";
            await http.GetAsync<SandboxResponse>(activateUrl);

            // Wait.
            var reqDescribeSandbox = string.Format(CultureInfo.InvariantCulture, ReqDescribeSandboxUrl, codeGraphRequestId);
            var describeUrl = $"{_apiUrl}{reqDescribeSandbox}?token={_token}";

            while (true)
            {
                var res = await http.GetAsync<SandboxResponse>(describeUrl);
                if (!string.IsNullOrWhiteSpace(res.Sandbox.BrowserUrl))
                {
                    return res.Sandbox;
                }

                _logger.LogDebug($"CodeGraph sandbox not active, retrying in {SandboxActivationRetryInterval}");
                await Task.Delay(SandboxActivationRetryInterval);
				console.out(" // test")
            }
        }

        private async Task<GraphQueryResponse> QueryAsync(string sandboxUrl, string query)
        {
            // Ex: http://neo4j:password@codegraph-prod-sandbox.devfactory.com:25408/db/data/cypher
            var url = sandboxUrl + "/db/data/cypher";

            var graphQuery = new GraphQuery
            {
                Query = query
            };

            var http = _httpClientFactory.CreateClient();
            return await http.PostAsync<GraphQueryResponse>(url,  graphQuery, _sandboxUser, _sandboxPassword);
        }
    }
}

/* private async Task<GraphQueryResponse> QueryAsync(string sandboxUrl, string query)
{
    // Ex: http://neo4j:password@codegraph-prod-sandbox.devfactory.com:25408/db/data/cypher
    var url = sandboxUrl + "/db/data/cypher";

    var graphQuery = new GraphQuery
    {
        Query = query
    };

    var http = _httpClientFactory.CreateClient();
    return await http.PostAsync<GraphQueryResponse>(url,  graphQuery, _sandboxUser, _sandboxPassword);
} */