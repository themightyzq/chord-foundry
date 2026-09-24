#include <juce_core/juce_core.h>

// Minimal console runner for the juce::UnitTest suite. Runs every registered
// test and returns a non-zero exit code if any assertion failed, so this can
// be wired up with add_test()/ctest.
int main(int, char**)
{
    juce::UnitTestRunner runner;
    runner.setAssertOnFailure(false);
    runner.runAllTests();

    int numFailures = 0;

    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (const auto* result = runner.getResult(i))
            numFailures += result->failures;
    }

    return numFailures == 0 ? 0 : 1;
}
