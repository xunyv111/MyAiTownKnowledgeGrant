import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const descriptorPath = path.join(root, "MyAiTownKnowledgeGrant.uplugin");
const changelogPath = path.join(root, "CHANGELOG.md");
const descriptor = JSON.parse(fs.readFileSync(descriptorPath, "utf8"));
const version = descriptor.VersionName;
const errors = [];

if (!version) errors.push("uplugin VersionName is empty");
if (!fs.readFileSync(changelogPath, "utf8").includes(`## ${version}`)) {
  errors.push(`CHANGELOG.md does not contain a ${version} section`);
}

const allowedExtensions = new Set([
  ".c", ".cc", ".cpp", ".cs", ".h", ".hpp", ".ini", ".json", ".md",
  ".mjs", ".txt", ".yml", ".yaml", ".html", ".css", ".js", ".ts"
]);
const skippedDirectories = new Set([
  ".git", ".github", "Binaries", "Intermediate", "Saved", "DerivedDataCache",
  "node_modules", "TestHost"
]);
const secretPatterns = [
  /sk-[A-Za-z0-9]{24,}/,
  /Bearer\s+[A-Za-z0-9._-]{24,}/,
  /(?:api[_-]?key|token)\s*[:=]\s*["'][^"']{16,}["']/i
];

function walk(directory, files) {
  for (const entry of fs.readdirSync(directory, { withFileTypes: true })) {
    if (skippedDirectories.has(entry.name)) continue;
    const fullPath = path.join(directory, entry.name);
    if (entry.isDirectory()) {
      walk(fullPath, files);
    } else if (allowedExtensions.has(path.extname(entry.name).toLowerCase())) {
      files.push(fullPath);
    }
  }
}

const files = [];
walk(root, files);
const findings = [];
for (const file of files) {
  const text = fs.readFileSync(file, "utf8");
  for (const pattern of secretPatterns) {
    pattern.lastIndex = 0;
    if (pattern.test(text)) {
      findings.push(path.relative(root, file));
      break;
    }
  }
}

if (findings.length > 0) {
  errors.push(`credential-like patterns detected: ${findings.join(", ")}`);
}

if (errors.length > 0) {
  console.error(errors.join("\n"));
  process.exit(1);
}

console.log(`Release contract valid: ${version}`);
