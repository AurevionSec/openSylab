import { useState, useRef, useCallback, useEffect } from 'react';

interface UseBarcodeOptions {
  onDetected: (code: string) => void;
}

interface UseBarcodeReturn {
  isSupported: boolean;
  isScanning: boolean;
  error: string | null;
  videoRef: React.RefObject<HTMLVideoElement | null>;
  startScan: () => Promise<void>;
  stopScan: () => void;
}

/**
 * Hook for barcode scanning via the BarcodeDetector Web API.
 * Falls back gracefully when BarcodeDetector is not supported.
 * Caller must render a <video ref={videoRef}> element for the camera preview.
 */
export function useBarcode({ onDetected }: UseBarcodeOptions): UseBarcodeReturn {
  const isSupported = typeof window !== 'undefined' && 'BarcodeDetector' in window;
  const [isScanning, setIsScanning] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const videoRef = useRef<HTMLVideoElement>(null);
  const streamRef = useRef<MediaStream | null>(null);
  const rafRef = useRef<number | null>(null);
  const detectorRef = useRef<BarcodeDetector | null>(null);

  const stopScan = useCallback(() => {
    if (rafRef.current !== null) {
      cancelAnimationFrame(rafRef.current);
      rafRef.current = null;
    }
    if (streamRef.current) {
      streamRef.current.getTracks().forEach(t => t.stop());
      streamRef.current = null;
    }
    setIsScanning(false);
  }, []);

  const startScan = useCallback(async () => {
    if (!isSupported) {
      setError('BarcodeDetector not supported in this browser.');
      return;
    }
    if (isScanning) return;
    setError(null);
    setIsScanning(true);

    try {
      const stream = await navigator.mediaDevices.getUserMedia({
        video: { facingMode: 'environment' },
      });
      streamRef.current = stream;

      if (!videoRef.current) {
        stream.getTracks().forEach(t => t.stop());
        streamRef.current = null;
        setIsScanning(false);
        return;
      }
      videoRef.current.srcObject = stream;
      await videoRef.current.play();

      if (!detectorRef.current) {
        detectorRef.current = new BarcodeDetector({ formats: ['code_128', 'qr_code', 'ean_13', 'code_39'] });
      }

      const detect = async () => {
        if (!videoRef.current || !detectorRef.current || !streamRef.current) return;
        try {
          const barcodes = await detectorRef.current.detect(videoRef.current);
          if (barcodes.length > 0) {
            stopScan();
            onDetected(barcodes[0].rawValue);
            return;
          }
        } catch {
          // Detection frame error — continue loop
        }
        rafRef.current = requestAnimationFrame(detect);
      };

      rafRef.current = requestAnimationFrame(detect);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Camera access denied.');
      setIsScanning(false);
    }
  }, [isSupported, isScanning, onDetected, stopScan]);

  useEffect(() => {
    return () => {
      stopScan();
    };
  }, [stopScan]);

  return { isSupported, isScanning, error, videoRef, startScan, stopScan };
}
