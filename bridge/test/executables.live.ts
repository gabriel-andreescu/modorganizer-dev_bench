import assert from "node:assert/strict";
import { liveClient } from "./live-client.ts";

type Entry = Record<string, string | number | boolean> & { title: string };
type Snapshot = { entries: Entry[] };
const client = await liveClient(process.argv[2]);
const title = "Dev Bench executable fixture";
const clone = `${title} copy`;
const operate = (action: string, args: Record<string, unknown> = {}) =>
  client.operate("executables", action, args);

async function snapshot() {
  return (await operate("snapshot")) as Snapshot;
}

const before = await snapshot();
assert.ok(before.entries.length > 0);
assert.ok(
  !before.entries.some(
    (entry) => entry.title === title || entry.title === clone,
  ),
);
const originalOrder = before.entries.map((entry) => entry.title);
const values: Record<string, unknown> = {
  title,
  binary: before.entries[0].binary,
  workingDirectory: before.entries[0].workingDirectory,
  arguments: "--automation-fixture",
  overwriteSteamAppID: true,
  steamAppID: "12345",
  useApplicationIcon: false,
  hide: true,
};
if ("minimizeToSystemTray" in before.entries[0]) {
  values.minimizeToSystemTray = true;
  values.useApplicationIcon = true;
}

try {
  await operate("create", { values });
  await operate("clone", {
    name: title,
    values: { title: clone, hide: false },
  });
  const order = [clone, title, ...originalOrder];
  await operate("setOrder", { names: order });
  const applied = await operate("apply");
  assert.equal(applied.unappliedChanges, false);
  await operate("accept");

  const saved = await snapshot();
  assert.deepEqual(
    saved.entries.map((entry) => entry.title),
    order,
  );
  const entry = saved.entries.find((item) => item.title === title)!;
  for (const [key, value] of Object.entries(values))
    assert.equal(entry[key], value, key);

  await operate("update", {
    name: title,
    values: { arguments: "discard this edit" },
  });
  await operate("cancel");
  assert.equal(
    (await snapshot()).entries.find((item) => item.title === title)!.arguments,
    values.arguments,
  );

  const libraries = [
    { process: "fixture.exe", library: "fixture.dll", enabled: false },
  ];
  const written = await operate("setLibraries", {
    name: title,
    libraries,
    enabled: false,
  });
  assert.deepEqual(written.libraries, libraries);
  assert.equal(written.enabled, false);
  assert.equal(
    (await snapshot()).entries.find((item) => item.title === title)!
      .forceLoadLibraries,
    false,
  );
  await operate("manage");
  await operate("accept");
  const reread = await operate("libraries", { name: title });
  assert.deepEqual(reread.libraries, libraries);
  assert.equal(reread.enabled, false);
} finally {
  await operate("cancel");
  const remaining = await snapshot();
  for (const name of [clone, title]) {
    if (remaining.entries.some((entry) => entry.title === name))
      await operate("remove", { name });
  }
  await operate("setOrder", { names: originalOrder });
  await operate("accept");
}

assert.deepEqual(
  (await snapshot()).entries.map((entry) => entry.title),
  originalOrder,
);
console.log(
  "Executable creation, fields, clone, order, persistence, cancellation and removal passed.",
);
