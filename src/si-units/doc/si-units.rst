.. include:: replace.txt
.. highlight:: cpp
.. highlight:: bash

International System of Units
-----------------------------

Motivation
**********
In 1998, NASA's Mars Climate Orbiter crashed because NASA used metric system while spacecraft
builder Lockheed Martin used US customary units. It was a USD 328 million project. And this is not
the only failure in government agencies and industries that are caused by unit mismatch.

Coherent use of units - dimensions, quantities, and scales - is imperative in correct physics equations,
robust software APIs, and trustworthy simulation results. Yet, it is easy to miss for untrained eyes.
Some of the popular mistakes include

  * Adding dBm_t and mWatt_t
  * Adding dB and dB
  * Treating MHz as if Hz
  * Multiplying dB with a number
  * Mixing linear scale variables and log scale variables without conversion

`Issue 649 <https://gitlab.com/nsnam/ns-3-dev/-/issues/649>` is one of such examples that were
detected and fixed in |ns3|.


Background
**********
International System of Units (SI Units) is the only system of coherent measurements. Although |ns3|
had already adopted it, enforcing and guaranteeing it in API coherence is yet to be fulfilled; rich
documentation and sharp human eyes are not enough. ``si-units`` module overcomes this limitation by
introducing compile-time guarantee in API-level coherence.


Design and Implementation considerations
****************************************
The implementation of ``si-units`` module does not distinguish idiosynchracies of `NIST
<www.nist.gov>`, `ISO <www.iso.org>`, or `IEC <www.iec.ch>`, although "National institute of
Standards and Technology's Guide for the Use of the International System of Units" is largely
followed.

In fact, if non-SI units such as miles, hour, degree, knot, and other `Imperial Units
<https://en.wikipedia.org/wiki/Imperial_units>` may well be implemented along as long as doing so
is useful and promotes convergence to `SI Units`.


SI rules takes precedence over |ns3|'s coding style or its idiomatic usages. The former is global
while the latter is relatively local. For example, a unit `dB` shall not be written as `DB`, `Db`,
or `db`. A symbol for a metric prefix "milli" shall be `m` and never `M`. A meter prefix mega's symbol
shall be `M` and never `m`. When spelled out, no space or hyphen shall be used. eg. `milligram`.
Neither `milli-gram` nor `milli_gram`. Neither CamelCase `MilliGram` nor camelCase `milliGram`.

``si-units`` module does not pursue to implement exhaustive SI units. Instead, decide what is useful
for |ns3| and implement that only. Same is applicable to operators for those units.

Behavioral correctness, ergonomic syntax, and compile-time validation are more emphasized while the
compile-and-run time performance is not dismissed.

What are possible patterns of coherence?
****************************************
There are couple of popular patterns with varying degree of robustness. As an example of the
underlying data type, below examples use ``double``, but it could be ``int32_t`` or any useful
choice.

A. Bad: Same-line comment in definitions
::

   double power1;                                   // mWatt. This comment is necessary
   double power2;                                   // dBm.   This comment is necessary
   double result = power1 + FromDbmToMWatt(power2); // mWatt. Note the use of CamelCase
   double result = FromMWattToDbm(power1) + power2; // dBm.   Note the use of CamelCase
   double result = power1 + power2;                 // valid in compile, invalid in physics

B. Good: Suffix in the names
::

   double powerMWatt;                                          // No need of a comment indicating a unit
   double powerDbm;                                            // No need of a comment indicating a unit
   double resultMWatt = powerMWatt + FromDbmToMWatt(powerDbm); // Note the use of CamelCase
   double resultDbm   = FromMWattToDbm(powerMWatt) + powerDbm; // Note the use of CamelCase
   double result      = powerMWatt + powerDbm;                 // valid in compile, invalid in physics


C. Better: Weak type aliases
::

   using   mWatt_t = double;                         // type alias. Helping readability, but not helping coherence
   using   dBm_u   = double;                         // type alias. Helping readability, but not helping coherence
   mWatt_t power1;                                   // No need of a comment indicating a unit
   dBm_u   power2;                                   // No need of a comment indicating a unit
   mWatt_t result = power1 + FromDbmToMWatt(power2); // mWatt. Note the use of CamelCase
   dBm_u   result = FromMWattToDbm(power1) + power2; // dBm.   Note the use of CamelCase
   auto    result = power1 + power2;                 // valid in compile, invalid in physics


D. Best: Strong types
::

   struct mWatt_t{..};
   struct dBm_t{..};
   mWatt_t  power1;
   dBm_t    power2;

   // Note operator overriding. Intuitive and robust. Valid in compile, valid in physics
   // Type deduction is valid depending on how operator overloadings are implemented
   mWatt_t result = power1 + power2; // mWatt
   dBm_t   result = power2 + power1; // dBm
   auto    result = power1 + power2; // mWatt
   auto    result = power2 + power1; // dBm

   auto    result = power1 - power2; // Compiler time failure, which is a desirable safe guard
                                     // unless the notion of subtracing energy is well-defined first.
                                     // This safeguard is infeasible in A, B, or C patterns

D pattern is the best since it adds the eyes of the compiler on top of what a human does and
guarantees the type matching, and validity of operations. Further, it eliminates cumbersome
conversions and allows direct translations of mathematical equations. The authors attain higher
confidence on the correctness on the implementations when the compiler okays it.


Postfix
*******
``si-units`` module enables naturally readable syntax.  Below are equivalent

::

  mWatt_t{100}
  100_mWatt

Conversions
***********
The strong types are data structures in disguise. They allow getters, setters, and useful converters
in a highly readable way. For example.

::

  mWatt_t power{100.0};
  static_assert(power.in_dBm() == 20_dBm);

It is straightforward to extend converters or useful utilities.


Attribute Systems
*****************
``si-units`` module supports |ns3| attribute system.


Working with legacy APIs
************************

It is recommended that every new code adopts strongly-typed SI units. The APIs and their
implementations will be naturally coherent. When the new code interfaces with the legacy code that do not
use strongly-typed SI units, apply conversions in calling the APIs and handling the return values. A
side benefit of this mindful integration is it provides an opportunity to document and validate the
behaviors of legacy APIs.

::

  // Legacy code taking frequency in MHz, returning power in dBm
  double SomeLegacyApi(double freq);

  // New code
  auto myFreq{2.4_GHz};                             // Hz
  auto xyz = dBm_t{SomeLegacyApi(myFreq.in_MHz())}; // dBm


Migration of legacy APIs
************************

A key is gradual, incremental, and testable refactoring. The outcome of the migration shall not
change the behaviors. If the migration efforts discover incumbent bugs, bug fixes should be handled
separately from API migration; Guaranteeing no behavioral changes during refactoring is as important
as fixing bugs.

::

  // Legacy API
  double SomeLegacyApi(double freq);

  // Improved API
  dBm_t SomeLegacyApi(Hz_t freq);

Note this API change does not necessarily imply the changes of implementation. The legacy
implementatinos may be preserved without a single change by making the new API wrap the legacy one.
This wrapper pattern adds confidence on the correctness, and allows gradual migration under a
controlled risk. Once you gain confidence, you can retire the legacy API and its implementation by
absorting the functionality in the new ones.

::

  // Legacy implementation
  double SomeLegacyApi(double freq)
  {
      ...
  }

  // Improved Implementation
  dBm_t SomeLegacyApi(Hz_t freq)
  {
    auto in = freq.in_MHz();
    auto out = SomeLegacyApi(in);
    return dBm_t{out}; // or combine all above lines into a single line.
  }

In some situations, adopting a strong-typed units can be prohibitive. There, two pass migration is
useful.

  * Pass 1: Replace standard types (eg. ``int8_t``, ``size_t``, ``double``) by weak-type aliases
    (eg. ``meter_u``, ``picoWatt_t``, ``joule_u``).
  * Pass 2: Replace weak-typed aliases by strong by strong types

::

  // Pass 0: Legacy API
  double SomeLegacyApi(double freq);

  // Pass 1: replace with weak-types
  using dBm_u = double;
  using Hz_u  = double;
  dBm_u SomeLegacyApi(Hz_u freq);

  // Pass 2: replace with strong-types
  struct dBm_t {..};
  struct Hz {..};
  dBm_t SomeLegacyApi(Hz_t freq);


Limitations
***********
``si-units`` module implements select SI units that are immediately useful for wired, wireless
communications and networking. Apparently, different use cases demand different SI units. In that
case, one should be able to extend in reference to existing modules.

Although users are presented strong types, their values are stored in fundamental data types. Some
are of an integral type, and some are of a floating type. Some integral types are signed and some
may be unsigned. Some integral types may use a 64 bit storage type, a smaller storage type, or a
platform-dependent one. These are chosen to satisfy typical daily needs. The choice of underlying
types may change as needs change.

The choice of underlying storage types in fact reflects a crucial design choice and inherent
limitations. For example, frequency type `Hz` can be designed with an understanding that the
simulators do not have a need to deal with sub-Hertz frequency such as `0.1 Hz`. Is this an
acceptable assumption? The answer depends on the use cases. Most of wireless communications deal
with kilo, mega, giga, terahertz carrier frequencies and bandwidths. Their Phased-lock loop drift
is sufficiently modeled in a unit Hertz. This is similar to that |ns3|'s time resolution is
nanoseconds by default, and no smaller than femtoseconds. Defining a minimum resolution makes it
possible to use an integral storage type instead of a floating-point one, effectively eliminating
doubts and fears of notorious precision and accuracy errors. As a result, ``si-units`` module
_envisions_ to use an integral type to implement `Hz`'s underlying storage.

But it implements a floating point type `double` instead. This is purely for easing the migration of
the legacy APIs, most of which are using `double`. (Some use integral types out of a questionable
design). Once their migration is complete, it is great to revisit `Hz_t` implementation and change
the underlying storage type to `int64_t`, without incurring large scale changes in the codebase.

Same idea can be applied to other SI unit implementations.


Validation
**********
`si-units-test-suite.cc` offers exhaustive unit tests.

::

  $ ./test.py -s si-units-test

or in DEBUG build profile,

::

  $ build/utils/ns3-dev-test-runner-default --verbose --test-name=si-units-test --stop-on-failure --fullness=QUICK
