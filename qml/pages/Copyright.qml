import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.All

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: copyrightText.y + copyrightText.height + Theme.paddingLarge

        Label {
            id: copyrightText
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.leftMargin: Theme.horizontalPageMargin
            anchors.rightMargin: Theme.horizontalPageMargin
            anchors.topMargin: Theme.paddingLarge + (page.orientation === Orientation.Portrait ? appWindow.screenCutoutHeight : 0)
            anchors.bottomMargin: Theme.paddingLarge
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeTiny
            wrapMode: Text.Wrap
            text: { return "<style>a:link { color: " + Theme.primaryColor + "; }</style>" +
                    "<p>Copyright (c) 2015 Christof Bürgi</p>" +
                    "<p>Copyright (c) 2024 Carmen F. B. (correction of settings saving)</p>" +
                    "<p>Copyright (c) 2026 Andrew Branson</p>" +
                    "<p>Permission is hereby granted, free of charge, to any person obtaining a copy " +
                    "of this software and associated documentation files (the &quot;Software&quot;), to deal " +
                    "in the Software without restriction, including without limitation the rights " +
                    "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell " +
                    "copies of the Software, and to permit persons to whom the Software is " +
                    "furnished to do so, subject to the following conditions:</p>" +
                    "<p>The above copyright notice and this permission notice shall be included in " +
                    "all copies or substantial portions of the Software.</p>" +
                    "<p>THE SOFTWARE IS PROVIDED &quot;AS IS&quot;, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR " +
                    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, " +
                    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE " +
                    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER " +
                    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, " +
                    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN " +
                    "THE SOFTWARE.</p><hr>" +
                    "<p>Swedish translation by Åke Engelbrektson, under the terms of CC-BY 3.0<br>" +
                    "Russian translation by mentaljam, under the terms of CC-BY 3.0<br>" +
                    "Hungarian translation by martonmiklos, under the terms of CC-BY 3.0<br>" +
                    "French translation by Arnaud Bérenger, under the terms of CC-BY 3.0<br>" +
                    "Spanish translation by Carmen F. B., under the terms of CC-BY 3.0<br>" +
                    "App icon courtesy of Moth, under the terms of CC-BY 3.0<br>" +
                    "see: <a href=\"http://creativecommons.org/licenses/by/3.0/\">http://creativecommons.org/licenses/by/3.0/</a></p><hr>" +
                    "<p>Contains parts of libzint, licensed as follows:</p>" +
                    "<p>libzint - the open source barcode library<br>" +
                    "Copyright (C) 2009 Robin Stuart &lt;robin@zint.org.uk&gt;<br>" +
                    "Copyright (C) 2006 Kentaro Fukuchi &lt;fukuchi@megaui.net&gt;<br>" +
                    "Copyright (C) 2004 Cliff Hones<br>" +
                    "Copyright (C) 2004 Grandzebu<br>" +
                    "Bug Fixes thanks to KL Chin &lt;klchin@users.sourceforge.net&gt;, Christian Sakowski and BogDan Vatra</p>" +
                    "<p>Redistribution and use in source and binary forms, with or without " +
                    "modification, are permitted provided that the following conditions " +
                    "are met:</p>" +
                    "<ol><li>Redistributions of source code must retain the above copyright " +
                    "notice, this list of conditions and the following disclaimer.</li>" +
                    "<li>Redistributions in binary form must reproduce the above copyright " +
                    "notice, this list of conditions and the following disclaimer in the " +
                    "documentation and/or other materials provided with the distribution.</li>" +
                    "<li>Neither the name of the project nor the names of its contributors " +
                    "may be used to endorse or promote products derived from this software " +
                    "without specific prior written permission.</li></ol>" +
                    "<p>THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS &quot;AS IS&quot; AND " +
                    "ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE " +
                    "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE " +
                    "ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE " +
                    "FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL " +
                    "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS " +
                    "OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) " +
                    "HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT " +
                    "LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY " +
                    "OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF " +
                    "SUCH DAMAGE.</p>"
            }
            textFormat: Text.RichText
            onLinkActivated: Qt.openUrlExternally(link)
        }

        VerticalScrollDecorator {}
    }
}
