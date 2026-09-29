import {
  resolveInstance,
  Unavailable,
  type Instance,
  type Target,
} from "./runtime.ts";

const metadataTimeoutMs = 3000;

export interface CatalogTool {
  name: string;
  description: string;
  inputSchema: Record<string, unknown>;
  readOnly?: boolean;
}

interface EventsResponse {
  headSeq: number;
  events: { seq: number; topic: string }[];
}

async function fetchJson(url: string, init?: RequestInit): Promise<unknown> {
  let response: Response;
  try {
    response = await fetch(url, init);
  } catch (error) {
    if ((error as Error).name === "AbortError") throw error;
    throw new Unavailable(
      `Dev Bench not reachable at ${url}: ${(error as Error).message}`,
    );
  }

  const body: unknown = await response.json().catch(() => undefined);
  if (!response.ok) {
    const message =
      (body as { error?: string } | undefined)?.error ?? response.statusText;
    throw new Error(`Dev Bench returned ${response.status}: ${message}`);
  }
  return body;
}

export async function listTools(target: Target): Promise<CatalogTool[]> {
  const { baseUrl } = await resolveInstance(target);
  const body = (await fetchJson(`${baseUrl}/api/tools`, {
    signal: AbortSignal.timeout(metadataTimeoutMs),
  })) as { tools?: CatalogTool[] };
  if (!Array.isArray(body.tools)) {
    throw new Error('Dev Bench /api/tools response missing a "tools" array');
  }
  return body.tools;
}

export async function callTool(
  target: Target,
  name: string,
  args: Record<string, unknown>,
  signal?: AbortSignal,
): Promise<unknown> {
  const { baseUrl, session } = await resolveInstance(target);
  return fetchJson(`${baseUrl}/api/tool/${encodeURIComponent(name)}`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      "X-Dev-Bench-Session": session,
    },
    body: JSON.stringify(args ?? {}),
    signal,
  });
}

export async function catalogChangedSince(
  instance: Instance,
  since: number,
): Promise<{ changed: boolean; headSeq: number }> {
  const body = (await fetchJson(
    `${instance.baseUrl}/api/events?since=${since}`,
    {
      signal: AbortSignal.timeout(metadataTimeoutMs),
    },
  )) as EventsResponse;
  const reset = body.headSeq < since;
  const missed =
    body.headSeq > since &&
    (body.events[0]?.seq ?? body.headSeq + 1) > since + 1;
  return {
    changed:
      reset ||
      missed ||
      body.events.some((event) => event.topic === "tools.changed"),
    headSeq: body.headSeq,
  };
}
