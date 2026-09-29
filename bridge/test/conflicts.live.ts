import assert from "node:assert/strict";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { join } from "node:path";
import { setTimeout } from "node:timers/promises";
import { liveClient } from "./live-client.ts";

type Mod = {
  name: string;
  path: string;
  priority: number;
  conflicts: { redundant: boolean };
};
type ConflictFile = { path: string; archive: boolean; providers: string[] };
type Conflict = {
  name: string;
  files: ConflictFile[];
  total: number;
  nextOffset: number | null;
};
type Inspection = {
  mods: Conflict[];
  totalMods: number;
  nextModOffset: number | null;
};
type Directory = {
  files: { path: string; hidden: boolean; directory: boolean }[];
};
type Dialog = {
  open: boolean;
  id: number;
  controls: { id: number; text?: string }[];
};
type Filters = { text: string; criteria: { index: number; state: string }[] };

const client = await liveClient(process.argv[2]);
const names = ["Dev Bench conflict fixture A", "Dev Bench conflict fixture B"];
const created: Mod[] = [];
const before = await client.call<{ mods: Mod[] }>("mods", { action: "list" });
assert.ok(!before.mods.some((mod) => names.includes(mod.name)));
const filters = await client.call<Filters>("mods", { action: "filters" });
let editorOpen = false;

async function until<T>(
  read: () => Promise<T>,
  ready: (value: T) => boolean,
): Promise<T> {
  const deadline = Date.now() + 15000;
  while (Date.now() < deadline) {
    const value = await read();
    if (ready(value)) return value;
    await setTimeout(50);
  }
  throw new Error("MO2 did not reach the expected state");
}

async function origins() {
  return client.call<{ origins: string[] }>("files", {
    action: "resolve",
    path: "bench-conflicts/common-a.txt",
  });
}

async function openEditor(name: string) {
  await client.call("mods", { action: "manage", name });
  await until(
    () => client.call<Dialog>("dialogs", { action: "describe" }),
    (dialog) => dialog.open,
  );
  editorOpen = true;
}

async function closeEditor() {
  await client.call("mods", { action: "closeEditor" });
  await until(
    () => client.call<{ state: string }>("mods", { action: "operationStatus" }),
    (result) => result.state === "finished",
  );
  editorOpen = false;
}

try {
  for (const name of names) {
    const mod = await client.call<Mod>("mods", { action: "create", name });
    created.push(mod);
    await mkdir(join(mod.path, "bench-conflicts"));
    for (const file of ["common-a.txt", "common-b.txt"]) {
      await writeFile(join(mod.path, "bench-conflicts", file), name);
    }
    await client.call("settings", { action: "refresh" });
    await until(
      () => client.call<{ mods: Mod[] }>("mods", { action: "list" }),
      (state) => state.mods.some((entry) => entry.name === mod.name),
    );
    await client.call("mods", {
      action: "setEnabled",
      name: mod.name,
      enabled: true,
    });
  }
  await until(origins, (state) => state.origins.length === 2);

  await client.call("mods", {
    action: "setFilters",
    filter: { text: "No fixture matches this text" },
  });
  const filtered = await client.call("mods", { action: "filters" });
  const page = await client.operate<Inspection>("conflicts", "inspect", {
    mods: names,
    modLimit: 1,
    limit: 1,
  });
  assert.equal(page.totalMods, 2);
  assert.equal(page.nextModOffset, 1);
  assert.equal(page.mods[0].total, 2);
  assert.equal(page.mods[0].nextOffset, 1);
  assert.equal(page.mods[0].files[0].archive, false);
  assert.equal(page.mods[0].files[0].providers.length, 2);
  assert.deepEqual(await client.call("mods", { action: "filters" }), filtered);

  const second = await client.operate<Inspection>("conflicts", "inspect", {
    mods: names,
    modOffset: 1,
    modLimit: 1,
    offset: 1,
    limit: 1,
  });
  assert.equal(second.nextModOffset, null);
  assert.equal(second.mods[0].name, names[1]);
  assert.equal(second.mods[0].nextOffset, null);
  assert.notEqual(second.mods[0].files[0].path, page.mods[0].files[0].path);

  const matching = await client.operate<Inspection>("conflicts", "inspect", {
    modFilter: "Dev Bench conflict fixture",
    filter: "common-a",
  });
  assert.equal(matching.totalMods, 2);
  assert.ok(
    matching.mods.every(
      (mod) => mod.total === 1 && mod.files[0].path.endsWith("common-a.txt"),
    ),
  );
  await client.call("mods", { action: "clearFilters" });

  const winner = (await origins()).origins[0];
  const winnerMod = created.find((mod) => mod.name === winner)!;
  const losing = created.find((mod) => mod.name !== winner)!;
  const redundant = await client.call<{ mods: Mod[] }>("mods", {
    action: "list",
    conflict: "redundant",
  });
  assert.ok(
    redundant.mods.some(
      (mod) => mod.name === losing.name && mod.conflicts.redundant,
    ),
  );
  const original = await readFile(
    join(winnerMod.path, "bench-conflicts/common-a.txt"),
  );
  await openEditor(winner);
  await client.operate("conflicts", "hide", {
    paths: ["bench-conflicts/common-a.txt"],
  });
  assert.deepEqual(
    await readFile(
      join(winnerMod.path, "bench-conflicts/common-a.txt.mohidden"),
    ),
    original,
  );
  assert.equal((await origins()).origins.length, 1);
  await closeEditor();
  await client.call("settings", { action: "refresh" });
  const nowContributing = await until(
    () => client.call<Mod>("mods", { action: "get", name: losing.name }),
    (mod) => !mod.conflicts.redundant,
  );
  assert.equal(nowContributing.conflicts.redundant, false);
  await openEditor(winner);

  let directory = await client.call<Directory>("conflicts", {
    action: "directory",
    path: "bench-conflicts",
  });
  assert.ok(
    directory.files.some(
      (file) => file.path.endsWith(".mohidden") && file.hidden,
    ),
  );
  await client.operate("conflicts", "unhide", {
    paths: ["bench-conflicts/common-a.txt.mohidden"],
  });
  assert.deepEqual(
    await readFile(join(winnerMod.path, "bench-conflicts/common-a.txt")),
    original,
  );
  assert.equal((await origins()).origins[0], winner);

  await client.operate("conflicts", "hideFiles", {
    paths: ["bench-conflicts"],
  });
  directory = await client.call<Directory>("conflicts", {
    action: "directory",
  });
  assert.ok(
    directory.files.some(
      (file) =>
        file.path === "bench-conflicts.mohidden" &&
        file.directory &&
        file.hidden,
    ),
  );
  await client.operate("conflicts", "unhide", {
    paths: ["bench-conflicts.mohidden"],
  });
  await closeEditor();
  assert.equal((await origins()).origins.length, 2);

  const winnerState = await client.call<Mod>("mods", {
    action: "get",
    name: winner,
  });
  await client.call("mods", {
    action: "setPriority",
    name: losing.name,
    priority: winnerState.priority,
  });
  await until(origins, (state) => state.origins[0] === losing.name);
} finally {
  if (editorOpen) await closeEditor();
  for (const mod of [...created].reverse()) {
    await client.call("mods", { action: "remove", name: mod.name });
    const dialog = await until(
      () => client.call<Dialog>("dialogs", { action: "describe" }),
      (state) => state.open,
    );
    const yes = dialog.controls.find(
      (control) => control.text?.replaceAll("&", "") === "Yes",
    );
    assert.ok(yes, JSON.stringify(dialog));
    await client.call("dialogs", { action: "click", id: yes.id });
    await until(
      () =>
        client.call<{ state: string; result?: { removed: boolean } }>("mods", {
          action: "operationStatus",
        }),
      (state) => state.state === "finished" && state.result?.removed === true,
    );
  }
  await client.call("mods", {
    action: "setFilters",
    filter: {
      text: filters.text,
      criteria: filters.criteria.map(({ index, state }) => ({ index, state })),
    },
  });
}

const after = await client.call<{ mods: Mod[] }>("mods", { action: "list" });
assert.deepEqual(
  after.mods.map((mod) => mod.name).sort(),
  before.mods.map((mod) => mod.name).sort(),
);
console.log(
  "Selected-mod inspection, filtering, pagination, provider changes, native hide/unhide and cleanup passed.",
);
