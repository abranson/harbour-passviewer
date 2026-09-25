import QtQuick 2.6
import QtTest 1.1
import "../qml/lib/utils.js" as Utils

TestCase {
    name: "PassBarcodeSelection"

    QtObject {
        id: codec

        function encodeMessage(message, encoding) {
            if (encoding === "invalid")
                return { error: "Unsupported encoding" }
            return { content: "encoded:" + message, encoding: encoding }
        }
    }

    function test_noBarcode() {
        compare(Utils.selectBarcode({}, codec).content, "")
        compare(Utils.selectBarcode({ barcodes: [] }, codec).content, "")
        compare(Utils.selectBarcode({ barcodes: [null, {}, { format: "unsupported" }] }, codec).content, "")
    }

    function test_skipMalformedAndUnsupported() {
        var result = Utils.selectBarcode({ barcodes: [null, {}, { format: "unsupported" },
            { format: "PKBarcodeFormatQR", message: 123 },
            { format: "PKBarcodeFormatAztec", message: "ticket", messageEncoding: "latin1" }] }, codec)
        compare(result.type, "aztec")
        compare(result.content, "encoded:ticket")
        compare(result.encoding, "latin1")
        compare(result.error, "")
    }

    function test_legacyBarcode() {
        var result = Utils.selectBarcode({ barcode: { format: "PKBarcodeFormatQR", message: "ticket" } }, codec)
        compare(result.type, "qr")
        compare(result.encoding, "iso-8859-1")
        compare(result.content, "encoded:ticket")
    }

    function test_firstSupportedAlternative() {
        var result = Utils.selectBarcode({ barcodes: [
            { format: "PKBarcodeFormatQR", message: "bad", messageEncoding: "invalid" },
            { format: "PKBarcodeFormatPDF417", message: "good" }] }, codec)
        compare(result.type, "pdf417")
        compare(result.error, "")
    }

    function test_invalidEncodingReported() {
        var result = Utils.selectBarcode({ barcode: {
            format: "PKBarcodeFormatQR", message: "bad", messageEncoding: "invalid" } }, codec)
        compare(result.content, "")
        compare(result.error, "Unsupported encoding")
    }

    function test_noStaleFields() {
        var result = Utils.selectBarcode({ barcodes: [] }, codec)
        compare(result.type, "")
        compare(result.encoding, "")
        compare(result.altText, "")
        compare(result.error, "")
    }
}
