import { App, Construct, Stack, StackProps } from "@aws-cdk/core";
import { TyhacStack } from "./tyhac-stack";

// for development, use account/region from cdk cli
const devEnv = {
  account: process.env.CDK_DEFAULT_ACCOUNT,
  region: process.env.CDK_DEFAULT_REGION,
};

const app = new App();

new TyhacStack(app, "tyhac-stack", { env: devEnv });

app.synth();
