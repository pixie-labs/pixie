import { fieldSortFunc } from './sort-funcs';

describe('fieldSortFunc', () => {
  it('correctly sorts the object by the specified subfield (ascending, with a value and record null)', () => {
    const f = fieldSortFunc('p99', true);
    const rows = [
      { p50: -20, p90: -26, p99: -23 },
      { p50: 10, p90: 60 },
      { p50: -30, p90: -36, p99: -33 },
      { p50: 20, p90: 260, p99: 261 },
      null,
      { p50: 30, p90: 360, p99: 370 },
    ];
    expect(rows.sort(f)).toStrictEqual([
      { p50: -30, p90: -36, p99: -33 },
      { p50: -20, p90: -26, p99: -23 },
      { p50: 20, p90: 260, p99: 261 },
      { p50: 30, p90: 360, p99: 370 },
      { p50: 10, p90: 60 },
      null,
    ]);
  });

  it('correctly sorts the object by the specified subfield (descending, with a record null)', () => {
    const f = fieldSortFunc('p99', false);
    const rows = [
      { p50: -20, p90: -26, p99: -23 },
      { p50: 10, p90: 60, p99: 89 },
      { p50: -30, p90: -36, p99: -33 },
      null,
      { p50: -10, p90: -6, p99: -3 },
      { p50: 30, p90: 360, p99: 370 },
    ];
    expect(rows.sort(f)).toStrictEqual([
      { p50: 30, p90: 360, p99: 370 },
      { p50: 10, p90: 60, p99: 89 },
      { p50: -10, p90: -6, p99: -3 },
      { p50: -20, p90: -26, p99: -23 },
      { p50: -30, p90: -36, p99: -33 },
      null,
    ]);
  });

  it('correctly sorts boolean values', () => {
    const f = fieldSortFunc('label', true);
    const rows = [
      { label: false },
      { label: true },
      { label: false },
      { label: true },
      { label: false },
      { label: false },
    ];
    expect(rows.sort(f)).toStrictEqual([
      { label: false },
      { label: false },
      { label: false },
      { label: false },
      { label: true },
      { label: true },
    ]);
  });

  it('correctly sorts bigint values', () => {
    const f = fieldSortFunc('label', true);
    const rows = [
      { label: BigInt(3) },
      { label: BigInt(1) },
      { label: BigInt(0) },
      { label: BigInt(2) },
      { label: BigInt(4) },
      { label: BigInt(5) },
    ];
    expect(rows.sort(f)).toStrictEqual([
      { label: BigInt(0) },
      { label: BigInt(1) },
      { label: BigInt(2) },
      { label: BigInt(3) },
      { label: BigInt(4) },
      { label: BigInt(5) },
    ]);
  });

  it('correctly sorts string values', () => {
    const f = fieldSortFunc('label', true);
    // The late Jurassic period, 164 to 145 million years ago, had very few (known) species considered to be dinosaurs.
    const rows = [
      /*
       * They _do_ move in herds! Nowadays, we don't have sauropods like this. At least giraffes are friendly!
       * Specimens have been found in Algeria, Portugal, Tanzania, and the United States.
       */
      { label: 'Brachiosaurus' },
      /*
       * The crocodile-sized gargoyle lizard lacks the morningstar at the tip of its Anklyosaurus cousin's tail.
       * Specimens have been found in the United States.
       */
      { label: 'Gargoyleosaurus' },
      /*
       * Visual evidence that chickens evolved from raptors. Feathered omnivores, just 1.7m long.
       * Specimens have been found in Canada.
       */
      { label: 'Chirostenotes' },
      /*
       * Tyrannosaurus Rex was not alive during the late Jurassic era. Its smaller cousin, Allosaurus, was.
       * Specimens have been found in Portugal and the United States.
       */
      { label: 'Allosaurus' },
      /*
       * A dwarf sauropod measuring about 6.2 meters snout to tail, with a recognizable bump atop its head.
       * Specimens have been found in Germany.
       */
      { label: 'Europasaurus' },
      /*
       * Also known as the "oak lizard", Dryosaurus is a bipedal dinosaur about four meters snout to tail.
       * Specimens have been found in Tanzania and the United States.
       */
      { label: 'Dryosaurus' },
    ];
    expect(rows.sort(f)).toStrictEqual([
      // Surprisingly, there are no dinosaurs from the late Jurassic period whose names start with an F.
      { label: 'Allosaurus' },
      { label: 'Brachiosaurus' },
      { label: 'Chirostenotes' },
      { label: 'Dryosaurus' },
      { label: 'Europasaurus' },
      { label: 'Gargoyleosaurus' },
    ]);
  });

  it('throws when trying to compare disparate types', () => {
    const f = fieldSortFunc('label', true);
    const rows = [
      { label: 'I am not a number.' },
      { label: 12345 },
    ];
    expect(() => rows.sort(f)).toThrowError();
  });

  it('throws when trying to compare types with no reasonable comparison method', () => {
    const f = fieldSortFunc('label', true);
    expect(() => [
      { label: function noSortingMe() {} },
      { label: function notMeEither() {} },
    ].sort(f)).toThrowError();

    expect(() => [
      { label: {} },
      { label: {} },
    ].sort(f)).toThrowError();

    expect(() => [
      { label: Symbol('Symbols cannot be compared reasonably') },
      { label: Symbol('Because doing so would be quite dirty') },
    ].sort(f)).toThrowError();
  });

  it('sorts nulls to the end of the list', () => {
    const f = fieldSortFunc('label', true);
    expect([
      { label: 'foo' },
      { label: null },
      { label: 'bar' },
    ].sort(f)).toStrictEqual([
      { label: 'bar' },
      { label: 'foo' },
      { label: null },
    ]);
  });
});