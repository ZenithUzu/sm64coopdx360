# xSDK C++23 hardware test

Standalone test; does not contain or boot sm64coopdx. Uses the xSDK GCC toolchain
and libxsdk.a, independently of the Microsoft SDK.

Run ./build.ps1 in PowerShell. Paths can be overridden with XsdkRoot, Toolchain,
Python and OutDir. Output: build/xsdk-test.xex. Generated files belong in build/.

On the console: blue normally, green while A is held, red while B is held;
B takes priority. Press Back to exit cleanly and flush the final log.
The runtime writes xsdk-test.log next to the XEX. The log checks C++ vector,
display setup, controller reads, frame submission, and detected A/B presses.
It cannot verify the visible image: that requires user observation.

Host compilation and XEX packaging passed. Console execution is pending.
