.pragma library

function timestamp(value) {
    if (!value)
        return 0;
    var time = new Date(value).getTime();
    return isFinite(time) ? time : 0;
}

function archiveKey(path, data) {
    // Issuer identity survives file moves and pass updates; cards have UUIDs.
    if (data.passTypeIdentifier && data.serialNumber)
        return "pass:" + JSON.stringify([data.passTypeIdentifier, data.serialNumber]);
    return path;
}

function classify(data, now, hoursBefore, hoursAfter, archiveAfterHours, archiveState) {
    var eventTime = timestamp(data.relevantDate);
    var expiryTime = timestamp(data.expirationDate);
    var expired = expiryTime !== 0 && expiryTime <= now;
    var past = eventTime !== 0 && eventTime + hoursAfter * 3600000 < now;
    // An explicit expiry takes precedence over the event/relevance date.
    var archiveTime = expiryTime || eventTime;
    var automatic = archiveAfterHours >= 0 && (data.voided === true
                    || (archiveTime !== 0 && archiveTime + archiveAfterHours * 3600000 <= now));
    var archived = archiveState === 1 || (archiveState !== 2 && automatic);
    return {
        eventTime: eventTime,
        expiryTime: expiryTime,
        archived: archived,
        past: past,
        inactive: past || expired || data.voided === true,
        current: !archived && !expired && data.voided !== true && eventTime !== 0
                 && eventTime - hoursBefore * 3600000 <= now && !past,
        section: eventTime === 0 ? "undated" : "timeline"
    };
}

function compare(a, b) {
    if (a.archived !== b.archived)
        return a.archived ? 1 : -1;
    // Undated passes form their own section; archived items share one timeline.
    if (!a.archived && a.timelineSection !== b.timelineSection)
        return a.timelineSection === "undated" ? -1 : 1;
    var difference = 0;
    if (a.archived) {
        difference = (b.eventTime || b.expiryTime || timestamp(b.mtime))
                   - (a.eventTime || a.expiryTime || timestamp(a.mtime));
    } else if (a.eventTime && b.eventTime) {
        difference = b.eventTime - a.eventTime;
    }
    return difference || a.name.localeCompare(b.name) || a.path.localeCompare(b.path);
}
