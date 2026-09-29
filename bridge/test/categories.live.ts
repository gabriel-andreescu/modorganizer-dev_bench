import assert from "node:assert/strict";
import { liveClient } from "./live-client.ts";

type Category = {
  row: number;
  id: number;
  name: string;
  parentId: number;
  nexus: unknown[];
};
type Snapshot = { categories: Category[] };
const client = await liveClient(process.argv[2]);
const operate = (action: string, args: Record<string, unknown> = {}) =>
  client.operate<Snapshot>("categories", action, args);
const parentName = "Dev Bench category fixture";
const childName = `${parentName} child`;
const renamed = `${parentName} renamed`;
const fixtureNames = new Set([parentName, childName, renamed]);
const before = await operate("list");
assert.ok(!before.categories.some((c) => fixtureNames.has(c.name)));
const identity = ({ row, ...category }: Category) => category;

try {
  let state = await operate("create", { values: { name: parentName } });
  const parent = state.categories.find((c) => c.name === parentName)!;
  assert.ok(parent);
  await operate("accept");

  state = await operate("create", {
    values: { name: childName, parentId: parent.id },
  });
  const child = state.categories.find((c) => c.name === childName)!;
  assert.equal(child.parentId, parent.id);
  state = await operate("update", {
    row: child.row,
    values: { name: renamed },
  });
  assert.ok(state.categories.some((c) => c.name === renamed));

  const ordered = [
    state.categories.find((c) => c.name === parentName)!,
    state.categories.find((c) => c.name === renamed)!,
    ...state.categories.filter((c) => !fixtureNames.has(c.name)),
  ];
  state = await operate("setOrder", { rows: ordered.map((c) => c.row) });
  assert.deepEqual(state.categories.map(identity), ordered.map(identity));
  await operate("accept");
  const saved = await operate("list");
  assert.deepEqual(saved.categories.map(identity), ordered.map(identity));

  state = await operate("manage");
  const savedChild = state.categories.find((c) => c.name === renamed)!;
  await operate("update", { row: savedChild.row, values: { name: childName } });
  await operate("cancel");
  assert.deepEqual(
    (await operate("list")).categories.map(identity),
    saved.categories.map(identity),
  );
} finally {
  await operate("cancel");
  let state = await operate("manage");
  for (const name of [childName, renamed, parentName]) {
    const entry = state.categories.find((c) => c.name === name);
    if (entry) state = await operate("remove", { row: entry.row });
  }
  await operate("accept");
}

assert.deepEqual(
  (await operate("list")).categories.map(identity),
  before.categories.map(identity),
);
console.log(
  "Category creation, parent relationship, rename, order, persistence, cancellation and removal passed.",
);
