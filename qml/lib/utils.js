.pragma library

function selectBarcode(pass, codec) {
    var barcodes = Array.isArray(pass.barcodes) ? pass.barcodes : [];
    if (barcodes.length === 0 && pass.barcode)
        barcodes = [pass.barcode];
    var error = "";
    for (var i = 0; i < barcodes.length; i++) {
        var barcode = barcodes[i];
        if (!barcode || typeof barcode.format !== "string")
            continue;
        var formats = { PKBarcodeFormatQR: "qr", PKBarcodeFormatAztec: "aztec",
                        PKBarcodeFormatPDF417: "pdf417", PKBarcodeFormatCode128: "code128" };
        var type = formats[barcode.format];
        if (!type)
            continue;
        if (typeof barcode.message !== "string") {
            error = qsTr("The barcode has no valid message.");
            continue;
        }
        var encoding = typeof barcode.messageEncoding === "string" ? barcode.messageEncoding : "iso-8859-1";
        var encoded = codec.encodeMessage(barcode.message, encoding);
        if (encoded.error) {
            error = encoded.error;
            continue;
        }
        return { type: type, encoding: encoded.encoding, content: encoded.content,
                 altText: typeof barcode.altText === "string" ? barcode.altText : "", error: "" };
    }
    return { type: "", encoding: "", content: "", altText: "", error: error };
}

function checkSpecificFields(pass, style, fieldType) {
    // adds undefined entries if necessary
    for (var field = 0; field < pass[style][fieldType].length; field++) {
        if (!('key' in pass[style][fieldType][field]))
            pass[style][fieldType][field].key = '';
        if (!('label' in pass[style][fieldType][field]))
            pass[style][fieldType][field].label = '';
        if (!('value' in pass[style][fieldType][field]))
            pass[style][fieldType][field].value = '';
    }
}

function checkFields(pass, style) {
    // adds undefined entries if necessary
    if (!('primaryFields' in pass[style]))
        pass[style].primaryFields = [];
    if (!('secondaryFields' in pass[style]))
        pass[style].secondaryFields = [];
    if (!('auxiliaryFields' in pass[style]))
        pass[style].auxiliaryFields = [];
    if (!('backFields' in pass[style]))
        pass[style].backFields = [];
    checkSpecificFields(pass, style, 'primaryFields');
    checkSpecificFields(pass, style, 'secondaryFields');
    checkSpecificFields(pass, style, 'auxiliaryFields');
    checkSpecificFields(pass, style, 'backFields');
}

function htmlescape(text) {
    // HTML escaping
    return text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
}


function setFields(pass, style, fieldType, target, dateTimeFormat, currencyFormat) {
    // sets a field model from json
    target.clear();
    var fields = [];
    if (style in pass && fieldType in pass[style]) {
        for (var field = 0; field < pass[style][fieldType].length; field++) {
            var data = pass[style][fieldType][field];
            // handle datetime fields
            if ("dateStyle" in data && "timeStyle" in data) {
                var dateFormat = data.dateStyle.substring(11).toLowerCase();
                var timeFormat = data.timeStyle.substring(11).toLowerCase();
                var ignoreTimeZone = false;
                if ('ignoresTimeZone' in data)
                    ignoreTimeZone = data.ignoresTimeZone;
                data.value = dateTimeFormat.format(data.value, dateFormat, timeFormat, ignoreTimeZone);
            }
            // handle currency fields
            else if ("currencyCode" in data) {
                var currencyCode = data.currencyCode.substring(0,3).toUpperCase();
                data.value = currencyFormat.format(data.value, currencyCode);
            }

            target.append({ field: String(data.key), title: String(data.label), value: htmlescape(String(data.value)) });
        }
    }
}

function interpretColor(color) {
    // turn passbook color descriptions into QML color descriptions
    if (color.substring(0,3) !== 'rgb')
        return color;
    var components = color.match(/\d+/g);
    var code = '#';
    for (var cur = 0; cur < 3; cur++) {
        var value = parseInt(components[cur]);
        if (isNaN(value) || value < 0)
            value = 0;
        if (value > 0xff)
            value = 0xff;
        if (value < 0x10)
            code += '0';
        code += value.toString(16);
    }
    return code;
}

function barcodeSize(origWidth, origHeight, maxWidth, em) {
    // find the optimal barcode size
    var width = 0;
    var height = 0;
    var scale = 0;
    // leave 0.5em border, but the barcode should be about 4em high
    while (width + em <= maxWidth && height < em * 4) {
        scale++;
        width = origWidth * scale;
        height = origHeight * scale;
    }
    if (width + em > maxWidth && scale > 1) {
        scale--;
        width = origWidth * scale;
        height = origHeight * scale;
    }
    return [width, height];
}

function copyText(text, clipboard, notificator) {
    clipboard.text = text;
    notificator.bannerNotification(qsTr("copied to clipboard"), "");
}

function sharedBarcodeImage(pass, path) {
    var info = pass.userInfo && pass.userInfo.passViewer;
    return info && info.barcodeImageVersion === 1
            ? "image://zipimage" + path + "/barcode.png" : "";
}
