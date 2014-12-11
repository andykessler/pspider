package com.akkessler.pspider;

public class pspider{
	public native static String nativeMain(String address, int depth, int thread_ct);

	static {
		System.loadLibrary("com_akkessler_pspider_pspider");
	}
}