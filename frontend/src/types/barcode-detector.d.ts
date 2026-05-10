// Type declaration for BarcodeDetector Web API (Shape Detection API)
interface BarcodeDetector {
  detect(image: HTMLVideoElement | HTMLImageElement | HTMLCanvasElement): Promise<Array<{ rawValue: string; format: string }>>;
}

declare var BarcodeDetector: {
  prototype: BarcodeDetector;
  new(options?: { formats?: string[] }): BarcodeDetector;
  getSupportedFormats(): Promise<string[]>;
};
