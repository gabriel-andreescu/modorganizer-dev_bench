export function printSetupSnippet(
  executablePath: string,
  scriptArgs: (string | undefined)[],
  args: { install?: string; instance?: string; pid?: number },
): void {
  const name = `mo2-dev-bench${args.pid === undefined ? "" : `-${args.pid}`}`;
  const cliArgs = [
    ...scriptArgs.filter(
      (argument): argument is string => argument !== undefined,
    ),
    ...(args.install ? ["--install", args.install] : []),
    ...(args.instance === undefined ? [] : ["--instance", args.instance]),
    ...(args.pid === undefined ? [] : ["--pid", String(args.pid)]),
  ];
  console.log(
    JSON.stringify(
      { mcpServers: { [name]: { command: executablePath, args: cliArgs } } },
      null,
      2,
    ),
  );
}
