#include <juce_core/juce_core.h>

#include <cstdio>

// Minimal console runner for the juce::UnitTest suites. With no argument it runs every
// registered suite; with one argument it runs only the suite of that name, which is how ctest
// gets one named result per suite. Returns non-zero if any assertion failed or no suite matched.
int main(int argc, char** argv)
{
    juce::Array<juce::UnitTest*> selected;

    for (auto* test : juce::UnitTest::getAllTests())
        if (argc < 2 || test->getName() == argv[1])
            selected.add(test);

    if (selected.isEmpty())
    {
        std::fprintf(stderr, "No test suite named '%s'\n", argc < 2 ? "" : argv[1]);
        return 2;
    }

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runTests(selected);

    int numFailures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (const auto* result = runner.getResult(i))
            numFailures += result->failures;
    }

    return numFailures == 0 ? 0 : 1;
}
