package com.aurelight.mgrk;

import android.app.Activity;
import android.os.SystemClock;
import android.util.Log;
import com.google.android.gms.ads.AdError;
import com.google.android.gms.ads.AdRequest;
import com.google.android.gms.ads.FullScreenContentCallback;
import com.google.android.gms.ads.LoadAdError;
import com.google.android.gms.ads.MobileAds;
import com.google.android.gms.ads.interstitial.InterstitialAd;
import com.google.android.gms.ads.interstitial.InterstitialAdLoadCallback;
import java.util.concurrent.atomic.AtomicBoolean;

/** Google demo ads only. Never replace this ID with a monetized unit for the title-entry placement. */
public final class BetaEntryAds {
    private static final String TAG = "RDEntryAds";
    private static final String DEMO_UNIT = "ca-app-pub-3940256099942544/1033173712";
    private static boolean initializing;
    private static boolean initialized;
    private static boolean loading;
    private static boolean showing;
    private static InterstitialAd ready;
    private static long loadedAt;

    private BetaEntryAds() {}

    // All state is accessed on the Activity UI thread.
    public static void prepare(Activity activity) {
        if (activity.isFinishing() || activity.isDestroyed()) return;
        if (!initialized) {
            if (initializing) return;
            initializing = true;
            new Thread(() -> {
                try {
                    MobileAds.initialize(activity.getApplicationContext(), status ->
                        activity.runOnUiThread(() -> {
                            initialized = true;
                            initializing = false;
                            prepare(activity);
                        }));
                } catch (RuntimeException error) {
                    activity.runOnUiThread(() -> { initializing = false; });
                    Log.w(TAG, "Demo SDK initialization failed; gameplay remains available.");
                }
            }, "RD-demo-ads-init").start();
            return;
        }
        if (loading || ready != null || showing) return;
        loading = true;
        try {
            InterstitialAd.load(activity.getApplicationContext(), DEMO_UNIT, new AdRequest.Builder().build(),
                new InterstitialAdLoadCallback() {
                    @Override public void onAdLoaded(InterstitialAd ad) {
                        loading = false;
                        ready = ad;
                        loadedAt = SystemClock.elapsedRealtime();
                        Log.i(TAG, "Demo entry ad ready");
                    }
                    @Override public void onAdFailedToLoad(LoadAdError error) {
                        loading = false;
                        ready = null;
                        Log.i(TAG, "Demo entry ad unavailable, code=" + error.getCode());
                    }
                });
        } catch (RuntimeException error) {
            loading = false;
            Log.w(TAG, "Demo ad load failed; gameplay remains available.");
        }
    }

    public static void show(Activity activity, Runnable continueEntry) {
        if (showing || activity.isFinishing() || activity.isDestroyed()) {
            continueEntry.run();
            return;
        }
        if (ready == null || SystemClock.elapsedRealtime() - loadedAt >= 3600000L) {
            ready = null;
            Log.i(TAG, "No ready demo ad: continuing entry immediately");
            continueEntry.run();
            prepare(activity);
            return;
        }
        InterstitialAd ad = ready;
        ready = null;
        showing = true;
        AtomicBoolean completed = new AtomicBoolean(false);
        Runnable finish = () -> {
            if (!completed.compareAndSet(false, true)) return;
            showing = false;
            Log.i(TAG, "Demo entry ad finished; continuing requested entry");
            continueEntry.run();
            prepare(activity);
        };
        ad.setFullScreenContentCallback(new FullScreenContentCallback() {
            @Override public void onAdShowedFullScreenContent() { Log.i(TAG, "Showing Google demo entry ad"); }
            @Override public void onAdDismissedFullScreenContent() { finish.run(); }
            @Override public void onAdFailedToShowFullScreenContent(AdError error) { finish.run(); }
        });
        try { ad.show(activity); }
        catch (RuntimeException error) { finish.run(); }
    }
}
