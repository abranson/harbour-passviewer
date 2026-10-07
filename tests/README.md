The tests use Qt Test and run without a display server. Build each in a separate
empty directory:

    qmake /path/to/harbour-passviewer/tests/storage.pro
    make && ./tst_savedcards

    qmake /path/to/harbour-passviewer/tests/icons.pro
    make && ./tst_cardicons

    qmake /path/to/harbour-passviewer/tests/import.pro
    make && ./tst_passimporter

    qmake /path/to/harbour-passviewer/tests/codec.pro
    make && QT_QPA_PLATFORM=offscreen ./tst_barcodecodec

    QT_QPA_PLATFORM=offscreen qmltestrunner -input tests/tst_passbarcode.qml

The codec tests require ZXing >= 3.0.2 with the new (Zint) writers, matching the
Sailfish OS 5.2 SDK. They check linear and matrix codes, rotation, Unicode,
binary payloads, GS1 identifiers, leading zeroes, PNG/JPEG screenshot imports,
and the imported-pass message-to-provider pipeline. Imported Aztec tests include
all256 bytes and a900-byte synthetic binary payload, alongside QR/PDF417/Code128,
UTF-8/Latin-1/Windows1252, invalid encoding rejection, and absence of unintended
GS1/ECI markers. The old bundled Zint encoder is no longer linked. QML tests
cover empty/malformed/unsupported barcode lists, legacy barcode fallback and
encoding failures. These synthetic cases are not validation of a real HVV ticket.
Compare an affected ticket's decoded bytes locally against the issuer's barcode
before relying on it; do not upload personal ticket data to a third party.
Storage tests use a
temporary data directory and verify persistence, deletion, private permissions,
invalid input and write failures.
They also check icon persistence, metadata, private permissions, invalid icon
rejection, resetting the icon, and preservation of the original barcode.
Icon tests cover square cropping at either edge of wide and tall images,
fitting with padding, and invalid input. They use local fixtures without network.
Import tests generate single and bundled ZIP passes. They check confirmation
staging/cancellation, snapshot consistency, source preservation, offline private
storage, no external discovery, duplicate replacement, malformed/conflicting
bundles, path traversal containment, failed writes and bounded/CRC-checked ZIP
extraction. No personal passes are used as fixtures.

On device, open both file types from Downloads with the app stopped and already
running, then accept and cancel imports. Try a file URL with spaces/percent/hash
characters. Check all bundle members appear after acceptance, reopen offline,
delete the source, and confirm the library still works. Reimport a newer version
and confirm one entry remains. Delete an imported pass and check the original
download remains. Verify .pkpasses is offered to Pass Viewer by the file manager.

On device, open Change icon from a saved card and from the list context menu.
Search for a brand (for example mcdonalds) and a general icon (for example train),
select a result, check its attribution and preview, then save. Check it appears
in the list, Archive, and card detail, including after restarting offline.
Choose an image, drag the square crop, toggle fit/crop, save, and remove the
source file: the saved icon should remain. Check picker cancel, no results,
connection failure, and Use default icon. Confirm the barcode still scans.

On-device checks: scan a real card, name and save it, reopen the app, display
it fullscreen in both orientations, and scan that display using another reader.
Also check camera permission failure, background/foreground transitions, leaving
the scanner, canceling before save, and long-press deletion with remorse.
Import a photo and a screenshot through the image picker, cancel the picker,
and try an image with no barcode to check the retry flow.

Camera gestures: pinch in/out and check the zoom stops at its minimum and the camera limit.
The in-frame zoom slider represents the supported range without labelling HAL
steps as magnification factors. Drag it, then pinch, then drag it again; each
control must update the camera and the thumb without jumping back. Slider
drags must not trigger tap-to-focus or scroll the page.
Tap off-center and verify the focus marker follows the tap, then disappears
after five seconds as continuous autofocus resumes. Backgrounding or leaving
the scanner must clear the focus lock. Only the visible central square is decoded.
Camera capture reads the rendered window to support GPU-only Sailfish camera
buffers. Check a known code visible in the preview reaches the naming page,
and that leaving/backgrounding the page suppresses any pending decode result.

Timeline and archive rules (Qt Quick Test, no display required):

    QT_QPA_PLATFORM=offscreen qmltestrunner -input tests/tst_timeline.qml

Build `tests/settings.pro` in a separate directory and run `tst_settingsstore`
to check that manual archive/restore choices survive reopening settings and
remain independent for different cards. These tests use temporary settings.

On device, check undated items appear first without section headers, descending event dates,
current-pass bold text, and expired/past entries in Archive. Long-press a card
to archive it, restart, then restore it from Archive; the card must remain
available without its data changing. Restoring an expired pass must not mark
it current. Check portrait cutout spacing and the landscape pass preview in
the main list. Archive status must survive a pass update or file move when the
issuer supplies a pass type and serial number.

Navigation: Archive must push a page, have no pull-down menu, and swipe back
to Pass Viewer. Open an archived item and return to Archive before returning
home. Copyright is in the Settings pull-down menu. Auto-archive after lives in
the Time section and must persist independently of highlight hours; Never must
keep even expired items in the main list unless manually archived.

Import: the single Import menu opens the camera with Choose image below
the preview. Check picker cancel resumes the camera, image selection decodes while the picker stays visible, then makes one back
transition directly to Save card without flashing the camera; decode errors
stay in the picker and allow retry (including choosing the same image again), and backgrounding during image decoding
does not navigate unexpectedly. Camera gestures must still work after cancel.

Import confirmation should render each complete pass, including images, fields
and barcode, before accepting. Cancelling a preview must not clear saved-pass
update markers. Check a multi-pass bundle scrolls cleanly to Import/Cancel.

Sharing export tests (same ZXing setup as codec tests):

    qmake /path/to/harbour-passviewer/tests/share.pro
    make && ./tst_passsharer

These use isolated temporary storage and verify binary payloads, standard
barcode fields, GS1/ECI and EAN image preservation, custom icon and attribution,
manifest hashes, private cache files, import round trips, unchanged original
packages, source removal, invalid input and write failures.
On device, open Share from main/Archive context menus and pass/card/simple-view
pulley menus. Check the Sailfish chooser opens; cancel without selecting a
recipient. Re-import an exported synthetic card and check its barcode also
opens fullscreen. Actual sending requires a user-selected destination.
