// WARNING: Do not modify! Generated file.

namespace UnityEngine.Purchasing.Security {
    public class GooglePlayTangle
    {
        private static byte[] data = System.Convert.FromBase64String("Y5b8W4hYgFMwSG1t3nwbMSCo9AkZqv/6PWMTSDGBCXcZFlxdxnuzdw6s3FEjtjXOOYTGlDBwFvDYjZkOCSUTHdFuAqBPjfjBeByfg30OQLRvHF2ksZlMyGlzfUa5UZj/FWUvtwTDwyZNqU+zBoHU8UZf6X0rDLGbxHb11sT58v3ecrxyA/n19fXx9PdJjeezCdPfo7pxDZRjINl6TKV5JYwSpMviGiO9s5TFAu5KL6RvG0ZvHz/zvq1ceugvbCtHaBpU8XqfqDvtvecxzHUKeGvsn/IuCFeWOQtffLbYIehlc1vjgC/22IXezKBs3sdRdvX79MR29f72dvX19AUB93NGqCny3Vb45KhLf0LU4d0fgegFUQNVXtZwB/nIGP8J0/b39fT1");
        private static int[] order = new int[] { 13,8,10,12,10,5,13,10,10,9,12,13,12,13,14 };
        private static int key = 244;

        public static readonly bool IsPopulated = true;

        public static byte[] Data() {
        	if (IsPopulated == false)
        		return null;
            return Obfuscator.DeObfuscate(data, order, key);
        }
    }
}
