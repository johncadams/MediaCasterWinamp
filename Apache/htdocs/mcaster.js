var selecteds   = new Array();
var stream      = true;
var tracksId    = "tracks";
var statusBarId = "statusBar";
var configId    = "configBar";
var searchForId = "searchFor";
var okBtnId     = "ok";
var clearBtnId  = "clear";
var downId      = "down";
var menuId      = "menu";
var module      = "tracks/";
var tracks;
var tbody;
var statusBar;
var searchFor;
var okBtn;
var clearBtn;
var menu;
var playItem;
var downItem;
var refreshItem;
var playlistItem;
var httpRequest;
var moving;
var holeIndex;
var holeOffX;
var holeOffY;


function doHilight(event) {
   var item = event.target;
   item.className += " hilight";
   item.style.cursor = 'pointer';
}


function doUnhilight(event) {
   var item = event.target;
   item.className = item.className.replace("hilight","");
   item.style.cursor = 'auto';
}


function doDepress(event) {
   var item = event.target;
   item.className += " depressed";
}


function doUndepress(event) {
   var item = event.target;
   item.className = item.className.replace("depressed","");
}


function doSort(event) {
   var col = event.target;
   if      (col.cellIndex==0) sorter(byArtist);
   else if (col.cellIndex==1) sorter(byTitle);
   else if (col.cellIndex==2) sorter(byAlbum);
   else if (col.cellIndex==3) sorter(byLength);
   else if (col.cellIndex==4) sorter(byTrack);
   else if (col.cellIndex==5) sorter(byGenre);
   else if (col.cellIndex==6) sorter(byYear);
   else if (col.cellIndex==7) sorter(byComment);
}


function byArtist(row1,row2) {
   return byAlpha(row1,row2,0);
}

function byTitle(row1,row2) {
   return byAlpha(row1,row2,1);
}

function byAlbum(row1,row2) {
   return byAlpha(row1,row2,2);
}

function byLength(row1,row2) {
   return byAlpha(row1,row2,9); // The hidden column
}

function byTrack(row1,row2) {
   return byNumeric(row1,row2,4);
}

function byGenre(row1,row2) {
   return byAlpha(row1,row2,5);
}

function byYear(row1,row2) {
   return byNumeric(row1,row2,6);
}

function byComment(row1,row2) {
   return byAlpha(row1,row2,7);
}

function byNumeric(row1, row2, col) {
   var cell1 = row1.cells[col].innerHTML;
   var cell2 = row2.cells[col].innerHTML;
   var rtn   = cell1 - cell2
   return rtn;
}

function byAlpha(row1, row2, col) {
   var cell1 = row1.cells[col].innerHTML;
   var cell2 = row2.cells[col].innerHTML;
// var rtn   = (cell1<cell2) ? -1 : (cell1>cell2) ? 1 : 0;
   var rtn   = (cell1 > cell2) - (cell2 > cell1);
   return rtn;
}


function sorter(sortFunc) {
   setBusy("Sorting...");

   var rows = new Array();
   while (tbody.rows.length>0) {
      rows.push( tbody.rows[0] );
      tbody.deleteRow(0);
   }

   rows.sort(sortFunc);

   for (var x=0; x<rows.length; x++) {
      var row = tbody.insertRow(-1);
      row.innerHTML = rows[x].innerHTML;
   }

   setItems(tbody.rows.length);
   setUnbusy();
}


function clearSearch() {
   searchFor.value = "";
   doSearchFor(searchFor);
}


function doKeyPress(event) {
   trace("doKeyPress: "+event);
   if (event.target==searchFor) {
      if (event.keyCode==13) { // Return
         okBtn.focus();
         doSearchFor();
      } else if (event.keyCode==27) { // Escape
         clearBtn.focus();
         doClear();
         clearMoving();
      }

   } else {
      // Escape
      if (event.keyCode==27) { // Escape
         hidePopup();

      } else if ((event.charCode==65||event.charCode==97) && event.ctrlKey) {
         // Ctrl-A
         clearSelected();
         for (i=0; i<tbody.rows.length; i++) {
            tbody.rows[i].className += " selected";
            selecteds.push(tbody.rows[i]);
         }
         event.stopPropagation();
         event.preventDefault();

      } else if (event.keyCode==38 || event.keyCode==40) { // Up/down
         var last = selecteds[selecteds.length-1];
         var row  = last.rowIndex-1;
         if (event.keyCode==38) row--;
         else                   row++;

         if (row<0 || row>=tracks.length) return;

         var tg = tracks[row].cells[0];
         var ev = document.createEvent("MouseEvents");
         ev.initMouseEvent("mousedown", true, true, window, 
                           0,0,0,0,0,0, false,false,false,0, tg);
         var foo = tg.dispatchEvent(ev);

      } else if (event.keyCode==13) { // Return
        doPlay();
      }
   }
}


function doClear() {
   searchFor.value = "";
   doSearchFor();
}


function doSearchFor() {
   var items  = 0;
   var string = searchFor.value.toUpperCase();

   setBusy("Searching: 0%");

   while (tbody.rows.length>0) {
      tbody.deleteRow(0);
   }

   if (string.length==0) {
      for (var x=0; x<tracks.length; x++) {
         var perc = Math.round(x*100/tracks.length);
         window.status = "Searching: "+perc+"%";
         var row = tbody.insertRow(-1);
         row.innerHTML = tracks[x].innerHTML;
         items++;
      }

   } else {
      for (var x=0; x<tracks.length; x++) {
         var artist  = tracks[x].cells[0];
         var title   = tracks[x].cells[1];
         var album   = tracks[x].cells[2];
//       var length  = tracks[x].cells[3];
//       var track   = tracks[x].cells[4];
         var genre   = tracks[x].cells[5];
//       var year    = tracks[x].cells[6];
         var comment = tracks[x].cells[7];

         if (artist .innerHTML.toUpperCase().indexOf(string)!=-1 || 
             title  .innerHTML.toUpperCase().indexOf(string)!=-1 || 
             album  .innerHTML.toUpperCase().indexOf(string)!=-1 || 
             genre  .innerHTML.toUpperCase().indexOf(string)!=-1 || 
             comment.innerHTML.toUpperCase().indexOf(string)!=-1) {

            var row = tbody.insertRow(-1);
            row.innerHTML = tracks[x].innerHTML;
            items++;
        }
     }
     var perc = Math.round(x*100/tracks.length);
     window.status = "Searching: "+perc+"%";
   }

   setItems(items);
   setUnbusy();
}


function fetchXml(url, callback, method) {
   try {
      document.body.style.cursor = 'wait';
      httpRequest = new XMLHttpRequest();
      httpRequest.overrideMimeType("text/xml");
   } catch (e) {
      try {
         httpRequest = new ActiveXObject("Msxml2.XMLHTTP");
      } catch (e) {
         try {
            httpRequest = new ActiveXObject("Microsoft.XMLHTTP");
         } catch (e) {
            document.body.style.cursor = 'auto';
            alert("Unable to send request: "+e);
            return false;
         }
      }
   }

   if (!method) method = "GET";
   httpRequest.open(method,url,true);
   httpRequest.onreadystatechange = callback;
   httpRequest.send(null);
}


function chkPermissions() {
   var row = tracks[0];
   var url = module + getMp3(row);
   setBusy("Checking...");
   fetchXml(url, handlePermissions, "HEAD");
}


function doOnLoad() {
   searchFor    = document.getElementById(searchForId);
   okBtn        = document.getElementById(okBtnId);
   clearBtn     = document.getElementById(clearBtnId);
   statusBar    = document.getElementById(statusBarId);
   menu         = document.getElementById(menuId);
   playItem     = document.getElementById("playItem");
   downItem     = document.getElementById("downItem");
   refreshItem  = document.getElementById("refreshItem");
   playlistItem = document.getElementById("playlistItem");


   doRefresh();
   doViaSelection();

   searchFor   .addEventListener('keypress',   doKeyPress,    false);
   okBtn       .addEventListener('click',      doSearchFor,   false);
   clearBtn    .addEventListener('click',      doClear,       false);
   menu        .addEventListener('contextmenu',doNothing,     false);
   playItem    .addEventListener("click",      doPlayItem,    false);
   downItem    .addEventListener("click",      doDownItem,    false);
   refreshItem .addEventListener("click",      doRefreshItem, false);
   playlistItem.addEventListener("click",      doPlaylistItem,false);

   var items = menu.getElementsByTagName("tr");
   for (var x=0; x<items.length; x++) {
      items[x].addEventListener('mouseover', doHilight,   false);
      items[x].addEventListener('mouseout',  doUnhilight, false);
   }
}


function doViaSelection() {
   var buttonBox  = document.getElementById("viaSelection");
   var buttons    = buttonBox.getElementsByTagName("input");
   var disabled   = (selecteds.length==0);
   for (x in buttons) {
      buttons[x].disabled = disabled;
   }
}


function getMp3(row) {
   var tds = row.cells;
   var td  = tds[ tds.length-1 ];
   var mp3 = tds[8].innerHTML;

   return mp3;
}


function getPlayURL() {
   var url     = module + "?action=enqueue";
   var bitrate = document.forms.config.bitrate.value;

   if (bitrate!=0) {
      url += "&bitrate=";
      url += bitrate;
   }

   for (i in selecteds) {
      var row = selecteds[i];
      var mp3 = getMp3(row);

      url += "&file=";
      url += escape(mp3);
   }
   return url;
}


function doPlay() {
   trace("doPlay");
   document.location.href = getPlayURL();
}


function doDownload() {
   alert("Not implemented");
}


function doRefresh() {
   setBusy("Downloading: 0%");
   fetchXml(module + "?action=library", handleRefresh);
}


function getConfigBar() {
   setBusy("");
   fetchXml(module + "?action=config", handleConfigBar);
}


function doNothing(event) {
   trace("doNothing: "+event);
   event.stopPropagation();
   event.preventDefault();
}


function doSelectRow(event) {
   trace("doSelectRow: "+event);
   var cell = event.target;
   var row  = cell.parentNode;
   var last;

   // We can get the scrollbar's events so skip them
   if (event.target.nodeName != "TD") return

   if (event.button == 2) { // We only care about right buttons
      showPopup(event);

   } else if (event.shiftKey) { // Shift select, hilight everything
      last = selecteds[selecteds.length-1];
      clearSelected();
      
      var incr = last.rowIndex<row.rowIndex?1:-1;
      for (i=last.rowIndex-1; i!=row.rowIndex-1; i+=incr) {
         tbody.rows[i].className   += " selected";
         selecteds.push(tbody.rows[i]);
      }
   } 

   if (!event.ctrlKey && !event.shiftKey) { // Normal select
      startMouseMove(event);
      while(selecteds.length>0) {
         var selected = selecteds[0];
         selected.className = selected.className.replace("selected","");
         selecteds.splice(0,1); // Remove it
      }
   }

   if (row.className!=null && row.className.indexOf("selected")!=-1) {
      row.className = row.className.replace("selected","");

   } else {
      row.className   += " selected";
      selecteds.push(row);
   }

   event.stopPropagation()
   event.preventDefault();
   doViaSelection();
}


function handleRefresh() {
   if (httpRequest.readyState == 4) {
      document.body.style.cursor = 'auto';
      if (httpRequest.status == 200) {
         try {
            var div       = document.getElementById(tracksId);
            div.innerHTML = httpRequest.responseText;

            if (!div.innerHTML) {
               throw "Missing HTTP/AJAX response text";
            }

            table         = div.getElementsByTagName  ("TABLE")[0];
            thead         = table.getElementsByTagName("THEAD")[0];
            tbody         = table.getElementsByTagName("TBODY")[0];
            rows          = tbody.rows;
            tracks        = new Array(rows.length);

            for (var x=0; x<rows.length; x++) {
               var perc      = Math.round(x*100/rows.length);
               window.status = "Downloading: "+perc+"%";
               tracks[x]     = rows[x];
            }

            thead   .addEventListener('mouseover',  doHilight,   false);
            thead   .addEventListener('mouseout',   doUnhilight, false);
            thead   .addEventListener('mousedown',  doDepress,   false);
            thead   .addEventListener('mouseup',    doUndepress, false);
            thead   .addEventListener('click',      doSort,      false);

            tbody   .addEventListener('mousedown',  doSelectRow, false);
            tbody   .addEventListener('mousemove',  doMouseMove, false);
            tbody   .addEventListener('mouseup',    doMouseUp,   false);

            document.addEventListener('contextmenu',doNothing,   false);
            document.addEventListener('keypress',   doKeyPress,  false);
//          document.addEventListener('mouseup',    hidePopup,   false);

            getConfigBar();
//          sorter(byArtist);
         } catch (e) {
            alert("Unable to refresh content: "+e);
         }

      } else {
         alert("Unable to fetch request");
         setUnbusy();
      }
      setUnbusy();
      setItems(tbody.rows.length);
   }
}


function handleConfigBar() {
   if (httpRequest.readyState == 4) {
      if (httpRequest.status == 200) {
         try {
            var elem = document.getElementById(configId);
            if (elem) elem.innerHTML = httpRequest.responseText;  						
            else      alert("Cannot locate: "+configId);
            chkPermissions();
         } catch (e) {
            alert("Unable to display content: "+e);
         }
      } else {
         alert("Unable to fetch request");
      }
      setUnbusy();
   }
}


function handlePermissions() {
   if (httpRequest.readyState == 4) {
      var downElem  = document.getElementById(downId);
      var downItem  = document.getElementById("downItem");
      if (httpRequest.status == 200) {
         if (downElem) downElem.className = downElem.className.replace("disable","");
         else          alert("Cannot find "+downId);
         if (downItem) downItem.className = downItem.className.replace("disable","");
         else          alert("Cannot find downItem");
      }
      setUnbusy();
   }
}


function showPopup(event) {
   trace("showPopup: "+event);
   menu.style.display = "block";
   menu.style.left    = event.clientX;
   menu.style.top     = event.clientY;
   menu.zIndex        = tbody.zIndex+1;
}


function hidePopup(event) {
   trace("hidePopup: "+event);
   menu.style.display = "none";
   menu.zIndex        = -1;
}


function doPlayItem(event) {
   trace("doPlayItem: "+event);
   doPlay();
   hidePopup();
   event.stopPropagation();
   event.preventDefault();
}


function doDownItem(event) {
   trace("doDownItem: "+event);
   doDownload();
   hidePopup();
   event.stopPropagation();
   event.preventDefault();
}


function doRefreshItem(event) {
   trace("doRefreshItem: "+event);
   doRefresh();
   hidePopup();
   event.stopPropagation();
   event.preventDefault();
}


function doPlaylistItem(event) {
   trace("doPlaylistItem: "+event);
   var url     = getPlayURL();
   var title   = "Media Caster Playlist";
   var link    = "<a href='"+url+"'>A special link awaits</a>";
   var headers = "content-type:text/html"
   var mailto  = "mailto:";

   mailto += "?headers="+escape(headers);
   mailto += "&subject="+escape(title);
   mailto += "&body="   +escape(link);

   document.location.href = mailto;
   hidePopup();
   event.stopPropagation();
   event.preventDefault();
}


function setItems(items) {
   statusBar.innerHTML = items + " items";
}


function setBusy(text) {
   document.body.style.cursor = 'wait';
   window.status = text;
}


function setUnbusy() {
   document.body.style.cursor = 'auto';
   window.status = "";
}


function clearSelected() {
   while(selecteds.length>0) {
      var selected = selecteds[0];
      selected.className = selected.className.replace("selected","");
      selecteds.splice(0,1); // Remove it
   }
}


function debug(obj) {
   var str = obj;
   for (item in obj) str += "<br>"+item+":"+obj[item];
   var windo = window.open("","height=500","new");
   windo.document.write(str);
// alert(str);
}

function trace(str) {
   return;
   if (true) {
      alert(str);
   } else {
      var windo = window.open("","height=500","new");
      windo.document.writeln(str);
      windo.document.writeln("<BR>");
   }
}


function getPosition(elem) {
   var offX = 0;
   var offY = 0;
      
   for (var self=elem; self; self=self.parentNode) {
      if (self.offsetTop) {
         offX += self.offsetLeft;
         offY += self.offsetTop;
      }
   }
   offY -= 100;
   return {x:offX,y:offY};
}


function startMouseMove(event) {
   if (!moving) {
      holeIndex   = event.target.parentNode.rowIndex;
      var hole    = tbody.rows[holeIndex-1];
      var holePos = getPosition(hole);
      var holeX0  = holePos.x;
      var holeY0  = holePos.y;
      var holeY1  = holeY0+hole.clientHeight;

      // Create the phony row
      moving                      = document.createElement("table");
      moving.className            = "drag content";
      moving.style.position       = "absolute";
      moving.style.width          = tbody.clientWidth;
      moving.style.visibility     = "hidden";
      moving.style.borderCollapse = "collapse";

      moving.addEventListener('mousemove',  doMouseMove, false);
      moving.addEventListener('mouseup',    doMouseUp,   false);
      moving.insertRow(0).innerHTML = hole.innerHTML;
      moving.rows[0].className += hole.className + " selected";
      document.body.appendChild(moving);

      // Blank out the real row and make the phony one the same size
      for (var i=0; i<hole.cells.length; i++) {
         moving.rows[0].cells[i].width  = hole.cells[i].clientWidth;
         moving.rows[0].cells[i].height = hole.cells[i].clientHeight;
      }

      holeOffX = event.clientX - holeX0;
      holeOffY = event.clientY - holeY0;
      moving.style.left = event.clientX - holeOffX;
      moving.style.top  = event.clientY - holeOffY;
      event.stopPropagation();
      event.preventDefault();
   }
}


function doMouseMove(event) {
   if (moving) {
      var hole    = tbody.rows[holeIndex-1];
      var holePos = getPosition(hole);
      var holeY0  = holePos.y;
      var holeY1  = holeY0 + hole.clientHeight;

      moving.style.left = holePos.x;  // Don't allow it to move L/R
      moving.style.top  = event.clientY - holeOffY;

      // We now know we're dragin' & dropin' so change the style
      moving.style.visibility = "visible";

      hole.className = hole.className.replace("selected",  "");
      hole.className = hole.className.replace("dropzone",  "");
      hole.className += " dropzone";

      if (event.clientY > holeY1 && holeIndex<=tbody.rows.length) {
         var nextIndex   = holeIndex+1;
         var next        = tbody.rows[nextIndex-1];
         var holeHTML    = hole.innerHTML;
         var nextHTML    = next.innerHTML;

         hole.innerHTML  = nextHTML;
         next.innerHTML  = holeHTML;
         hole.className  = next.className.replace("dropzone", "");
         next.className += "dropzone";

         holeIndex++;

      } else if (event.clientY < holeY0 && holeIndex>1) {
         var prevIndex   = holeIndex-1;
         var prev        = tbody.rows[prevIndex-1];
         var holeHTML    = hole.innerHTML;
         var prevHTML    = prev.innerHTML;

         hole.innerHTML  = prevHTML;
         prev.innerHTML  = holeHTML;
         hole.className  = prev.className.replace("dropzone", "");
         prev.className += "dropzone";

         holeIndex--;
      }
   }
   event.stopPropagation();
   event.preventDefault();
}


function doMouseUp(event) {
   clearMoving();
   event.stopPropagation();
   event.preventDefault();
}


function clearMoving() {
   if (moving) {
      var hole = tbody.rows[holeIndex-1];

      moving.style.visibility = "hidden";
      hole.className = "selected";
      selecteds.push(hole);

      moving.removeEventListener('mousemove', doMouseMove, false);
      moving.removeEventListener('mouseup',   doMouseUp,   false);

      document.body.removeChild(moving);
      moving    = null;
      holeIndex = null;
   }
}
