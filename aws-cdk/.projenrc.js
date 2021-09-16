const { AwsCdkTypeScriptApp, ProjectType } = require("projen");
const project = new AwsCdkTypeScriptApp({
  cdkVersion: "1.118.0",
  defaultReleaseBranch: "main",
  name: "tyhac-aws-hackster",
  cdkVersionPinning: true,
  devDeps: ["prettier"],
  eslintOptions: {
    prettier: true,
  },
  cdkDependencies: [
    "@aws-cdk/aws-iam",
    "@aws-cdk/aws-s3",
    "@aws-cdk/aws-lambda",
    "@aws-cdk/aws-iot",
    "@aws-cdk/aws-dynamodb",
    "@aws-cdk/aws-lambda-event-sources",
  ],
  authorEmail: "github@talkncloud.com",
  authorName: "mick jacobsson",
  description: "pretty awesome little backend work to support tyhac ",
  keywords: ["tyhac", "covid", "cdk", "iac", "hackster"],
  repository: "https://github.com/talkncloud/tyhac-aws-hackster",
  context: {
    "@aws-cdk/core:newStyleStackSynthesis": true,
    "@aws-cdk/core:enableStackNameDuplicates": "true",
    "aws-cdk:enableDiffNoFail": "true",
    "@aws-cdk/core:stackRelativeExports": "true",
  },
  tsconfig: {
    compilerOptions: { noUnusedLocals: false },
  },
  projectType: ProjectType.APP,
  gitignore: [".DS_Store"],
  github: false
});
project.synth();
