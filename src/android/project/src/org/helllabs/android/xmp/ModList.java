/* Xmp for Android
 * Copyright (C) 2010 Claudio Matsuoka
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

package org.helllabs.android.xmp;

import java.io.File;
import java.io.FilenameFilter;

import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ListView;

public class ModList extends PlaylistActivity {
	static final int SETTINGS_REQUEST = 45;
	Xmp xmp = new Xmp();	/* used to get mod info */
	boolean isBadDir = false;
	ProgressDialog progressDialog;
	final Handler handler = new Handler();
	String currentDir;
	int directoryNum;
	
	class ModFilter implements FilenameFilter {
	    public boolean accept(File dir, String name) {
	    	//Log.v(getString(R.string.app_name), "** " + dir + "/" + name);
	        return (new File(dir,name).isDirectory()) || InfoCache.testModule(dir + "/" + name);
	    }
	}
	
	@Override
	public void onCreate(Bundle icicle) {
		setContentView(R.layout.modlist);
		super.onCreate(icicle);			
		registerForContextMenu(getListView());
		final String media_path = prefs.getString(Settings.PREF_MEDIA_PATH, Settings.DEFAULT_MEDIA_PATH);
		
		// Check if directory exists
		final File modDir = new File(media_path);
		
		if (!modDir.isDirectory()) {
			final Examples examples = new Examples(this);
			
			isBadDir = true;
			AlertDialog alertDialog = new AlertDialog.Builder(this).create();
			
			alertDialog.setTitle("Oops");
			alertDialog.setMessage(media_path + " not found. " +
					"Create this directory or change the module path.");
			alertDialog.setButton("Create", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int which) {
					examples.install(media_path,
							prefs.getBoolean(Settings.PREF_EXAMPLES, true));
					updatePlaylist(media_path);
				}
			});
			alertDialog.setButton2("Back", new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog, int which) {
					finish();
				}
			});
			alertDialog.show();
			return;
		}
		
		updatePlaylist(media_path);
	}

	public void updatePlaylist(final String path) {
		modList.clear();
		
		currentDir = path;
		setTitle(path);
		
		isBadDir = false;
		progressDialog = ProgressDialog.show(this,      
				"Please wait", "Scanning module files...", true);

		directoryNum = 1;	// we always have ..
		final File modDir = new File(path);
		new Thread() { 
			public void run() {
				modList.add(new PlaylistInfo("[..]", "Parent directory", path + "/..", R.drawable.parent));
            	for (File file : modDir.listFiles(new ModFilter())) {
            		if (file.isDirectory()) {
            			directoryNum++;
            			modList.add(new PlaylistInfo("[" + file.getName() + "]", "Directory",
            								file.getAbsolutePath(), R.drawable.folder));
            		}
            	}
            	for (File file : modDir.listFiles(new ModFilter())) {
            		if (file.isFile()) {
            			ModInfo m = InfoCache.getModInfo(path + "/" + file.getName());
            			modList.add(new PlaylistInfo(m.name, m.chn + " chn " + m.type, m.filename, -1));
            		}
            	}
            	
                final PlaylistInfoAdapter playlist = new PlaylistInfoAdapter(ModList.this,
                			R.layout.song_item, R.id.info, modList);
                
                /* This one must run in the UI thread */
                handler.post(new Runnable() {
                	public void run() {
                		 setListAdapter(playlist);
                	 }
                });
            	
                progressDialog.dismiss();
			}
		}.start();
	}
	
	@Override
	protected void onListItemClick(ListView l, View v, int position, long id) {
		String name = modList.get(position).filename;
		File file = new File(name);
		if (file.isDirectory()) {
			updatePlaylist(name);
		} else {
			playModule(name);
		}
	}
	
	@Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
    	//Log.v(getString(R.string.app_name), requestCode + ":" + resultCode);
    	switch (requestCode) {
    	case SETTINGS_REQUEST:
            if (isBadDir || resultCode == RESULT_OK)
            	updatePlaylist(currentDir);
            super.showToasts = prefs.getBoolean(Settings.PREF_SHOW_TOAST, true);
            break;
        }
    }

	
	// Playlist context menu
	
	@Override
	public void onCreateContextMenu(ContextMenu menu, View v, ContextMenuInfo menuInfo) {
		AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo)menuInfo;
		int i = 0;
		menu.setHeaderTitle("Add to playlist");
		if (info.position < directoryNum) {
			menu.add(Menu.NONE, i, i, "Files to new playlist");
			for (String s : PlaylistUtils.listNoSuffix()) {
				i++;
				menu.add(Menu.NONE, i, i, "Add to " + s);
			}			
		} else {
			menu.add(Menu.NONE, i, i, "New playlist...");
			for (String each : PlaylistUtils.listNoSuffix()) {
				i++;
				menu.add(Menu.NONE, i, i, "Add to " + each);
			}
		}
	}
	
	@Override
	public boolean onContextItemSelected(MenuItem item) {
		AdapterView.AdapterContextMenuInfo info = (AdapterView.AdapterContextMenuInfo)item.getMenuInfo();
		int index = item.getItemId();
		
		if (info.position < directoryNum) {			// Directories
			PlaylistUtils p = new PlaylistUtils();
			if (index == 0) {
				p.filesToNewPlaylist(this, modList.get(info.position).filename, null);
			} else {
				index--;
				p.filesToPlaylist(this, modList.get(info.position).filename, PlaylistUtils.listNoSuffix()[index]);
			}
		} else {				// Files
			if (index == 0) {						// Files
				(new PlaylistUtils()).newPlaylist(this);
				return true;
			}
			index--;
			PlaylistInfo pi = modList.get(info.position);
			String line = pi.filename + ":" + pi.comment + ":" + pi.name;
			PlaylistUtils.addToList(this, PlaylistUtils.listNoSuffix()[index], line);
		}

		return true;
	}


	// Menu
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
	    MenuInflater inflater = getMenuInflater();
	    inflater.inflate(R.menu.options_menu, menu);
	    return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		switch(item.getItemId()) {
		case R.id.menu_prefs:		
			startActivityForResult(new Intent(this, Settings.class), SETTINGS_REQUEST);
			break;
		case R.id.menu_refresh:
			updatePlaylist(currentDir);
			break;
		}
		return true;
	}
}