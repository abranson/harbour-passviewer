import QtQuick 2.6
import QtTest 1.1
import "../qml/lib/timeline.js" as Timeline

TestCase {
    name: "Timeline"

    property double now: Date.parse("2026-09-24T12:00:00Z")

    function classify(data, manual) {
        return Timeline.classify(data, now, 4, 4, 4, manual || 0)
    }

    function test_undatedAndInvalidDates() {
        compare(classify({}).section, "undated")
        verify(!classify({}).archived)
        compare(classify({ relevantDate: "invalid" }).section, "undated")
        verify(!classify({ relevantDate: "invalid", expirationDate: "invalid" }).archived)
    }

    function test_currentAndGraceBoundaries() {
        verify(classify({ relevantDate: "2026-09-24T16:00:00Z" }).current)
        verify(!classify({ relevantDate: "2026-09-24T16:00:01Z" }).current)
        verify(classify({ relevantDate: "2026-09-24T08:00:01Z" }).current)
        verify(classify({ relevantDate: "2026-09-24T08:00:00Z" }).archived)
        verify(classify({ relevantDate: "2026-09-24T14:00:00+02:00" }).current)
    }

    function test_expiredAndVoided() {
        verify(classify({ expirationDate: "2026-09-24T08:00:00Z" }).archived)
        verify(!classify({ expirationDate: "2026-09-24T08:00:01Z" }).archived)
        verify(classify({ voided: true }).archived)
        verify(!classify({ relevantDate: "2026-09-24T12:00:00Z", voided: true }).current)
    }

    function test_manualArchiveAndRestore() {
        verify(classify({}, 1).archived)
        verify(!classify({}, 2).archived)
        var expired = { relevantDate: "2026-09-24T12:00:00Z", expirationDate: "2026-09-24T11:00:00Z" }
        verify(!classify(expired, 2).archived)
        verify(!classify(expired, 2).current)
        verify(!classify({ relevantDate: "2026-09-20T12:00:00Z" }, 2).archived)
    }

    function test_archivePreference() {
        var old = { relevantDate: "2026-09-20T12:00:00Z" }
        var result = Timeline.classify(old, now, 4, 4, -1, 0)
        verify(result.past)
        verify(!result.archived)
        verify(!result.current)
    }

    function test_independentArchiveDelay() {
        var old = { relevantDate: "2026-09-24T00:00:00Z" }
        var waiting = Timeline.classify(old, now, 4, 4, 24, 0)
        verify(waiting.past)
        verify(!waiting.current)
        verify(!waiting.archived)
        verify(Timeline.classify(old, now, 4, 4, 12, 0).archived)
        verify(Timeline.classify({ relevantDate: "2026-09-24T12:00:00Z" }, now, 4, 4, 0, 0).archived)
    }

    function test_neverAndExplicitExpiry() {
        var expired = { expirationDate: "2026-09-20T12:00:00Z", voided: true }
        verify(!Timeline.classify(expired, now, 4, 4, -1, 0).archived)
        verify(Timeline.classify(expired, now, 4, 4, -1, 1).archived)
        var validUntilTomorrow = { relevantDate: "2026-09-20T12:00:00Z", expirationDate: "2026-09-25T12:00:00Z" }
        verify(!Timeline.classify(validUntilTomorrow, now, 4, 4, 0, 0).archived)
    }

    function row(name, data, manual) {
        var state = classify(data, manual)
        return { name: name, path: name, archived: state.archived,
                 timelineSection: state.section, eventTime: state.eventTime,
                 expiryTime: state.expiryTime, mtime: new Date(now) }
    }

    function test_timelineOrder() {
        var rows = [row("Old", { relevantDate: "2026-09-20T12:00:00Z" }),
                    row("Current", { relevantDate: "2026-09-24T12:00:00Z" }),
                    row("Undated", {}),
                    row("Future", { relevantDate: "2026-09-27T12:00:00Z" }),
                    row("Older", { relevantDate: "2026-09-19T12:00:00Z" })]
        rows.sort(function(a, b) { return Timeline.compare(a, b, 0) })
        compare(rows.map(function(value) { return value.name }).join(","), "Undated,Future,Current,Old,Older")
    }

    function test_stableIdentity() {
        var data = { passTypeIdentifier: "issuer", serialNumber: "123" }
        compare(Timeline.archiveKey("old.pkpass", data), Timeline.archiveKey("new.pkpass", data))
        verify(Timeline.archiveKey("card:one", {}) !== Timeline.archiveKey("card:two", {}))
    }
}
