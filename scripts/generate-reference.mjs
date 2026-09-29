import { readFile, writeFile } from "node:fs/promises";
import { createRequire } from "node:module";
import { fileURLToPath } from "node:url";

const require = createRequire(new URL("../bridge/package.json", import.meta.url));
const prettier = require("prettier");
const config = await prettier.resolveConfig(
  fileURLToPath(new URL("../README.md", import.meta.url)),
);

const [input = new URL("../bridge/src/tools-fallback.json", import.meta.url), output = new URL("../docs/automation/tools.md", import.meta.url)] =
  process.argv.slice(2);
const catalog = JSON.parse(await readFile(input, "utf8"));
const sections = [
  "# Tool reference",
  "Generated from the shared native tool descriptors. All arguments and results are JSON. Use the same tool names over MCP or `POST /api/tool/<name>`.",
];
for (const tool of catalog.tools) {
  const fields = tool.inputSchema.properties;
  sections.push(`## ${tool.name}`, tool.description);
  const discriminator = fields.action ? "action" : "kind";
  const label = discriminator === "action" ? "Actions" : "Kinds";
  sections.push(`${label}: ${fields[discriminator].enum.map((value) => `\`${value}\``).join(", ")}.`);
  const rows = Object.entries(fields).filter(([name]) => name !== discriminator);
  if (rows.length) {
    sections.push(
      "| Argument | Type | Description |\n| --- | --- | --- |\n" +
        rows
          .map(
            ([name, schema]) =>
              `| \`${name}\` | ${schema.type ?? "JSON value"}${schema.items ? ` of ${schema.items.type}` : ""} | ${(schema.description ?? "").replaceAll("|", "\\|")} |`,
          )
          .join("\n"),
    );
  }
  sections.push(
    "<details>\n<summary>Input schema</summary>\n\n```json\n" +
      JSON.stringify(tool.inputSchema, null, 2) +
      "\n```\n\n</details>",
  );
}
await writeFile(
  output,
  await prettier.format(sections.join("\n\n") + "\n", { ...config, parser: "markdown" }),
);
